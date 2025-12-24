package com.robcholz.lumen.client;

import com.robcholz.lumen.client.config.LumenConfigScreen;
import com.terraformersmc.modmenu.api.ConfigScreenFactory;
import com.terraformersmc.modmenu.api.ModMenuApi;

public class LumenModMenuIntegration implements ModMenuApi {
    @Override
    public ConfigScreenFactory<?> getModConfigScreenFactory() {
        return LumenConfigScreen::new;
    }
}
