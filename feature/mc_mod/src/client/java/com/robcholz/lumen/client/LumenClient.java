package com.robcholz.lumen.client;

import com.robcholz.lumen.LumenSyncServer;
import net.fabricmc.api.ClientModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class LumenClient implements ClientModInitializer {
    public static final String MOD_ID = "lumen";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    @Override
    public void onInitializeClient() {
        try {
            LumenSyncServer.start();
        } catch (Exception e) {
            LOGGER.error("Failed to start Lumen sync HTTP server", e);
        }
        LOGGER.info("Lumen client ready");
    }
}
