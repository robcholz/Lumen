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

#include "include/i2c_bus.hpp"

#include <mutex>

#include <esp_log.h>

#include "include/pins.hpp"

static constexpr auto I2C_BUS_TAG = "[lumen:i2c_bus]";

std::once_flag S_INIT_FLAG;
esp_err_t S_INIT_STATUS = ESP_FAIL;
i2c_master_bus_handle_t S_BUS_HANDLE = nullptr;

void initBusOnce() {
    constexpr i2c_master_bus_config_t busCfg = {
            .i2c_port = PIN_I2C_PORT,
            .sda_io_num = PIN_NUM_I2C_SDA,
            .scl_io_num = PIN_NUM_I2C_SCL,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {.enable_internal_pullup = 1, .allow_pd = 0},
    };

    S_INIT_STATUS = i2c_new_master_bus(&busCfg, &S_BUS_HANDLE);
    if (S_INIT_STATUS != ESP_OK) {
        ESP_LOGE(I2C_BUS_TAG, "Failed to init I2C bus: %s", esp_err_to_name(S_INIT_STATUS));
    } else {
        ESP_LOGI(I2C_BUS_TAG, "Initialized shared I2C bus on port %d", static_cast<int>(PIN_I2C_PORT));
    }
}


namespace lumen::i2c {
    esp_err_t init_shared_bus() {
        std::call_once(S_INIT_FLAG, initBusOnce);
        return S_INIT_STATUS;
    }

    i2c_master_bus_handle_t get_shared_bus_handle() {
        if (init_shared_bus() != ESP_OK) {
            return nullptr;
        }
        return S_BUS_HANDLE;
    }
} // namespace lumen::i2c
