package com.robcholz.lumen;

import com.google.gson.JsonObject;
import com.sun.net.httpserver.Headers;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.OutputStream;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.nio.charset.StandardCharsets;
import java.util.Enumeration;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class LumenSyncServer {
    private static final Logger LOGGER = LoggerFactory.getLogger(LumenSyncServer.class);
    private static final int PORT = 47123;

    private static HttpServer server;

    private LumenSyncServer() {
    }

    public static synchronized void start() throws IOException {
        if (server != null) {
            return;
        }

        InetAddress bindAddress = resolveBindAddress();
        server = HttpServer.create(new InetSocketAddress(bindAddress, PORT), 0);
        server.createContext("/lumen/sync", LumenSyncServer::handleSync);
        server.createContext("/lumen/sync/skin", LumenSyncServer::handleSkin);
        ExecutorService executor = Executors.newSingleThreadExecutor(r -> {
            Thread thread = new Thread(r, "lumen-sync-http");
            thread.setDaemon(true);
            return thread;
        });
        server.setExecutor(executor);
        server.start();
        String host = server.getAddress().getAddress().getHostAddress();
        LOGGER.info("Lumen sync HTTP server listening on http://{}:{}/lumen/sync", host, PORT);
        LOGGER.info("Lumen sync HTTP server listening on http://{}:{}/lumen/sync/skin", host, PORT);
    }

    private static void handleSync(HttpExchange exchange) throws IOException {
        try {
            if (!"GET".equalsIgnoreCase(exchange.getRequestMethod())) {
                sendJson(exchange, 405, "{\"error\":\"method_not_allowed\"}");
                return;
            }

            LumenSyncState.Snapshot snapshot = LumenSyncState.requestSnapshot(false);
            JsonObject payload = new JsonObject();
            payload.addProperty("mode", snapshot.mode());
            payload.addProperty("health", snapshot.health());
            payload.addProperty("max_health", snapshot.maxHealth());
            sendJson(exchange, 200, payload.toString());
        } catch (Exception e) {
            LOGGER.warn("Failed to handle /lumen/sync request", e);
            sendJson(exchange, 500, "{\"error\":\"server_error\"}");
        } finally {
            exchange.close();
        }
    }

    private static void handleSkin(HttpExchange exchange) throws IOException {
        try {
            if (!"GET".equalsIgnoreCase(exchange.getRequestMethod())) {
                sendJson(exchange, 405, "{\"error\":\"method_not_allowed\"}");
                return;
            }

            LumenSyncState.Snapshot snapshot = LumenSyncState.requestSnapshot(true);
            JsonObject payload = new JsonObject();
            payload.addProperty("skin_width", snapshot.skinWidth());
            payload.addProperty("skin_height", snapshot.skinHeight());
            payload.addProperty("skin", snapshot.skinBase64());
            sendJson(exchange, 200, payload.toString());
        } catch (Exception e) {
            LOGGER.warn("Failed to handle /lumen/sync/skin request", e);
            sendJson(exchange, 500, "{\"error\":\"server_error\"}");
        } finally {
            exchange.close();
        }
    }

    private static void sendJson(HttpExchange exchange, int status, String body) throws IOException {
        Headers headers = exchange.getResponseHeaders();
        headers.set("Content-Type", "application/json; charset=utf-8");
        byte[] payload = body.getBytes(StandardCharsets.UTF_8);
        exchange.sendResponseHeaders(status, payload.length);
        try (OutputStream output = exchange.getResponseBody()) {
            output.write(payload);
        }
    }

    private static InetAddress resolveBindAddress() throws IOException {
        Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
        while (interfaces.hasMoreElements()) {
            NetworkInterface iface = interfaces.nextElement();
            if (!iface.isUp() || iface.isLoopback() || iface.isVirtual()) {
                continue;
            }
            Enumeration<InetAddress> addresses = iface.getInetAddresses();
            while (addresses.hasMoreElements()) {
                InetAddress address = addresses.nextElement();
                if (address instanceof Inet4Address && !address.isLoopbackAddress()) {
                    return address;
                }
            }
        }
        LOGGER.warn("No non-loopback IPv4 address found; binding to 0.0.0.0");
        return InetAddress.getByName("0.0.0.0");
    }
}
