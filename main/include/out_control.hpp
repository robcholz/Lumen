/*
Lumen
Copyright (C) 2025  Finn Sheng

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#ifndef MAIN_INCLUDE_OUT_CONTROL_HPP
#define MAIN_INCLUDE_OUT_CONTROL_HPP

#include <driver/gpio.h>

#include "pins.hpp"

static constexpr auto OUT_CONTROL_TAG = "[lumen:out_control]";

inline void controlInit() {
    gpio_reset_pin(PIN_NUM_OUT_CONTROL);

    gpio_config_t ioConf = {};
    ioConf.pin_bit_mask = 1ULL << PIN_NUM_OUT_CONTROL;
    ioConf.mode = GPIO_MODE_OUTPUT;
    ioConf.pull_up_en = GPIO_PULLUP_DISABLE;
    ioConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ioConf.intr_type = GPIO_INTR_DISABLE;

    const esp_err_t err = gpio_config(&ioConf);
    ESP_LOGI(OUT_CONTROL_TAG, "gpio_config(%d) = %s", PIN_NUM_OUT_CONTROL, esp_err_to_name(err));
}

inline void controlTurnOn() {
    gpio_set_level(PIN_NUM_OUT_CONTROL, 1);
}

inline void controlTurnOff() {
    gpio_set_level(PIN_NUM_OUT_CONTROL, 0);
}

#endif // MAIN_INCLUDE_OUT_CONTROL_HPP
