package com.robcholz.lumen.client.config;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.Reader;
import java.io.Writer;
import java.nio.file.Files;
import java.nio.file.Path;

public class LumenConfig {
    private static final Logger LOGGER = LoggerFactory.getLogger("lumen");
    private static final Gson GSON = new GsonBuilder().setPrettyPrinting().create();

    public String portPath = "";
    public boolean autoReconnect = true;
    public int reconnectPeriodSeconds = 5;

    public static LumenConfig load(Path path) {
        if (!Files.exists(path)) {
            return new LumenConfig();
        }
        try (Reader reader = Files.newBufferedReader(path)) {
            LumenConfig config = GSON.fromJson(reader, LumenConfig.class);
            if (config == null) {
                return new LumenConfig();
            }
            config.normalize();
            return config;
        } catch (IOException e) {
            LOGGER.warn("Failed to load config from {}", path, e);
            return new LumenConfig();
        }
    }

    public LumenConfig copy() {
        LumenConfig copy = new LumenConfig();
        copy.portPath = portPath;
        copy.autoReconnect = autoReconnect;
        copy.reconnectPeriodSeconds = reconnectPeriodSeconds;
        return copy;
    }

    public void save(Path path) {
        normalize();
        try {
            Files.createDirectories(path.getParent());
            try (Writer writer = Files.newBufferedWriter(path)) {
                GSON.toJson(this, writer);
            }
        } catch (IOException e) {
            LOGGER.warn("Failed to save config to {}", path, e);
        }
    }

    private void normalize() {
        if (portPath == null) {
            portPath = "";
        }
        if (reconnectPeriodSeconds < 1) {
            reconnectPeriodSeconds = 1;
        }
    }
}
