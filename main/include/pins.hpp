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

#ifndef MAIN_INCLUDE_PINS_HPP
#define MAIN_INCLUDE_PINS_HPP

#include <driver/gpio.h>

#define LCD_HOST SPI2_HOST // SPI2 for general peripheral
#define PIN_NUM_MOSI GPIO_NUM_3
#define PIN_NUM_SCLK GPIO_NUM_10
#define PIN_NUM_CS GPIO_NUM_4
#define PIN_NUM_DC GPIO_NUM_1
#define PIN_NUM_RST GPIO_NUM_9
#define PIN_NUM_BK GPIO_NUM_2

#define PIN_I2C_PORT I2C_NUM_0
#define PIN_NUM_I2C_SDA GPIO_NUM_8
#define PIN_NUM_I2C_SCL GPIO_NUM_7
#define I2C_FREQ 400000

#define PIN_NUM_BUZZER GPIO_NUM_5
#define PIN_NUM_OUT_CONTROL GPIO_NUM_6

#define PIN_NUM_EC_SW GPIO_NUM_20
#define PIN_NUM_EC_A GPIO_NUM_0
#define PIN_NUM_EC_B GPIO_NUM_21

#endif // MAIN_INCLUDE_PINS_HPP
