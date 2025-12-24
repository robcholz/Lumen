package com.robcholz.lumen.client;

import com.robcholz.lumen.LumenSyncState;
import com.robcholz.lumen.client.config.LumenConfig;
import com.robcholz.lumen.client.config.LumenConfigManager;
import net.fabricmc.api.ClientModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class LumenClient implements ClientModInitializer {
    public static final String MOD_ID = "lumen";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
    private static final ScheduledExecutorService SERIAL_EXECUTOR =
            Executors.newSingleThreadScheduledExecutor(r -> {
                Thread thread = new Thread(r, "lumen-serial-sync");
                thread.setDaemon(true);
                return thread;
            });

    private static void sendPlayerInfo(LumenSerialManager serial) {
        try {
            LumenSyncState.Snapshot snapshot = LumenSyncState.requestPlayerSnapshot();
            String json = "{\"mode\":\"" + snapshot.mode() + "\",\"health\":" +
                    snapshot.health() + ",\"max_health\":" + snapshot.maxHealth() + "}";
            serial.send("sync", json.getBytes(StandardCharsets.UTF_8));
        } catch (Exception e) {
            LOGGER.debug("Failed to send player info over serial", e);
        }
    }

    private static void sendSkinInfo(LumenSerialManager serial) {
        try {
            LumenSyncState.SkinPayload payload = LumenSyncState.requestSkinPayload();
            byte[] data = payload.toWireBytes();
            if (data.length == 0) {
                return;
            }
            serial.send("sync/skin", data);
        } catch (Exception e) {
            LOGGER.debug("Failed to send skin info over serial", e);
        }
    }

    @Override
    public void onInitializeClient() {
        LumenConfig config = LumenConfigManager.get();
        String portPath = SerialPortLocator.resolvePortPath(config);
        if (portPath == null || portPath.isBlank()) {
            LOGGER.warn("No serial port configured; set it in Mod Menu or via -Dlumen.serialPort/LUMEN_SERIAL_PORT");
        }

        LumenSerialManager serial = new LumenSerialManager();
        SERIAL_EXECUTOR.scheduleAtFixedRate(
                () -> sendPlayerInfo(serial),
                0,
                1,
                TimeUnit.SECONDS
        );
        SERIAL_EXECUTOR.scheduleAtFixedRate(
                () -> sendSkinInfo(serial),
                0,
                20,
                TimeUnit.SECONDS
        );

        LOGGER.info("Lumen client ready");
    }
}
