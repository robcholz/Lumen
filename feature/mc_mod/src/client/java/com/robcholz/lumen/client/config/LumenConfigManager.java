package com.robcholz.lumen.client.config;

import net.fabricmc.loader.api.FabricLoader;

import java.nio.file.Path;

public final class LumenConfigManager {
    private static final String FILE_NAME = "lumen.json";
    private static final Path CONFIG_PATH = FabricLoader.getInstance().getConfigDir().resolve(FILE_NAME);
    private static volatile LumenConfig config = LumenConfig.load(CONFIG_PATH);

    private LumenConfigManager() {
    }

    public static LumenConfig get() {
        return config;
    }

    public static void save(LumenConfig updated) {
        config = updated;
        updated.save(CONFIG_PATH);
    }
}
