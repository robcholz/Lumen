package com.robcholz.lumen.client;

import com.robcholz.lumen.SerialPackClient;
import com.robcholz.lumen.client.config.LumenConfig;
import com.robcholz.lumen.client.config.LumenConfigManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;

public final class LumenSerialManager implements AutoCloseable {
    private static final Logger LOGGER = LoggerFactory.getLogger("lumen");

    private SerialPackClient client;
    private String connectedPortPath;
    private long lastReconnectAttemptMillis;
    private boolean hasAttemptedInitialConnect;

    public void send(String path, byte[] data) {
        if (!ensureConnected()) {
            return;
        }
        try {
            client.send(path, data);
        } catch (IOException e) {
            LOGGER.debug("Serial send failed, closing connection", e);
            closeClient();
        }
    }

    private boolean ensureConnected() {
        LumenConfig config = LumenConfigManager.get();
        String desiredPortPath = SerialPortLocator.resolvePortPath(config);
        if (desiredPortPath == null || desiredPortPath.isBlank()) {
            return false;
        }
        if (client != null && !client.isOpen()) {
            closeClient();
        }
        if (client != null && !desiredPortPath.equals(connectedPortPath)) {
            closeClient();
        }
        if (client != null) {
            return true;
        }

        long now = System.currentTimeMillis();
        if (hasAttemptedInitialConnect) {
            if (!config.autoReconnect) {
                return false;
            }
            long intervalMillis = Math.max(1, config.reconnectPeriodSeconds) * 1000L;
            if (now - lastReconnectAttemptMillis < intervalMillis) {
                return false;
            }
        }

        hasAttemptedInitialConnect = true;
        lastReconnectAttemptMillis = now;
        try {
            client = new SerialPackClient(desiredPortPath);
            connectedPortPath = desiredPortPath;
            return true;
        } catch (IOException e) {
            LOGGER.debug("Failed to connect to serial port {}", desiredPortPath, e);
            closeClient();
            return false;
        }
    }

    private void closeClient() {
        if (client != null) {
            client.close();
        }
        client = null;
        connectedPortPath = null;
    }

    @Override
    public void close() {
        closeClient();
    }
}
