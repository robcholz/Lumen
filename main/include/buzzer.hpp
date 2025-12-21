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

#ifndef MAIN_INCLUDE_BUZZER_HPP
#define MAIN_INCLUDE_BUZZER_HPP

#include <driver/ledc.h>

#include "pins.hpp"

inline void buzzerInit() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    constexpr ledc_timer_config_t tcfg = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_10_BIT, // 0–1023
            .timer_num = LEDC_TIMER_0,
            .freq_hz = 2000, // 2 kHz default
            .clk_cfg = LEDC_AUTO_CLK
    };

    constexpr ledc_channel_config_t ccfg = {
            .gpio_num = PIN_NUM_BUZZER,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0, // start muted
            .hpoint = 0
    };
#pragma GCC diagnostic pop

    ledc_timer_config(&tcfg);

    ledc_channel_config(&ccfg);
}

/// @param freq hz
/// @param duty 0-1024
inline void buzzerTone(const uint32_t freq, const uint16_t duty) {
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

inline void buzzerOff() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

#endif // MAIN_INCLUDE_BUZZER_HPP
