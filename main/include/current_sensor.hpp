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

#ifndef MAIN_INCLUDE_CURRENT_SENSOR_HPP
#define MAIN_INCLUDE_CURRENT_SENSOR_HPP

#include <bitset>
#include <utility>

#include <esp_err.h>
#include <esp_log.h>

#include <ina226_interface.h>

#include "i2c_bus.hpp"
#include "pins.hpp"

inline INA226* CURRENT_SENSOR;

static constexpr auto ADDRESS = 0x44;
static constexpr auto CURRENT_SENSOR_TAG = "[lumen:current_sensor]";

inline void currentSensorInit() {
    const auto bus = lumen::i2c::get_shared_bus_handle();
    if (bus == nullptr) {
        ESP_LOGE(CURRENT_SENSOR_TAG, "Shared I2C bus is not available");
        return;
    }

    CURRENT_SENSOR = new INA226{bus, ADDRESS, I2C_FREQ};

    CURRENT_SENSOR->Calibrate(100, 1.6);
    CURRENT_SENSOR->SetOperatingMode(INA226::OperatingMode::SHUNT_AND_BUS_CONTINUOUS);
    CURRENT_SENSOR->SetAveragingMode(INA226::AveragingMode::SAMPLE_1);
    CURRENT_SENSOR->SetBusVoltageConversionTime(INA226::ConversionTime::TIME_8244_uS);
    CURRENT_SENSOR->SetShuntVoltageConversionTime(INA226::ConversionTime::TIME_8244_uS);
}

inline float currentSensorReadVoltage() {
    return CURRENT_SENSOR->GetBusVoltage_mV();
}

inline float currentSensorReadCurrent() {
    return CURRENT_SENSOR->GetCurrent_uA() / 1000.f;
}

inline float currentSensorReadPower() {
    return CURRENT_SENSOR->GetPower_uW() / 1000.f;
}

[[maybe_unused]]
inline void currentSensorReadDebug() {
    ESP_LOGI(CURRENT_SENSOR_TAG, "\n");
    ESP_LOGI(CURRENT_SENSOR_TAG, "Shunt voltage: %" PRIi32 " uV", CURRENT_SENSOR->GetShuntVoltage_uV());
    ESP_LOGI(CURRENT_SENSOR_TAG, "Bus voltage raw: %d", CURRENT_SENSOR->GetBusVoltage_Raw());
    ESP_LOGI(CURRENT_SENSOR_TAG, "Bus voltage: %d mV", currentSensorReadVoltage());
    ESP_LOGI(CURRENT_SENSOR_TAG, "Current: %f mA", currentSensorReadCurrent());
    ESP_LOGI(CURRENT_SENSOR_TAG, "Power: %f mW", currentSensorReadPower());
    ESP_LOGI(CURRENT_SENSOR_TAG, "Config: %" PRIx16, CURRENT_SENSOR->GetConfig());
    ESP_LOGI(CURRENT_SENSOR_TAG, "Operating mode: %" PRIu8, std::to_underlying(CURRENT_SENSOR->GetOperatingMode()));
    ESP_LOGI(CURRENT_SENSOR_TAG, "Averaging mode: %" PRIu8, std::to_underlying(CURRENT_SENSOR->GetAveragingMode()));
    ESP_LOGI(
            CURRENT_SENSOR_TAG,
            "Bus voltage conversion time: %" PRIu8,
            std::to_underlying(CURRENT_SENSOR->GetBusVoltageConversionTime())
    );
    ESP_LOGI(
            CURRENT_SENSOR_TAG,
            "Shunt voltage conversion time: %" PRIu8,
            std::to_underlying(CURRENT_SENSOR->GetShuntVoltageConversionTime())
    );
}

[[maybe_unused]]
inline void currentSensorScan() {
    auto bus = lumen::i2c::get_shared_bus_handle();
    if (bus == nullptr) {
        ESP_LOGE(CURRENT_SENSOR_TAG, "Shared I2C bus is not available");
        return;
    }

    constexpr auto addr = ADDRESS;
    if (const esp_err_t ok = i2c_master_probe(bus, addr, 50); ok == ESP_OK) {
        ESP_LOGI(CURRENT_SENSOR_TAG, "Found I2C device at 0x%02X\n", addr);
    } else {
        ESP_LOGW(CURRENT_SENSOR_TAG, "Probe failed for I2C device at 0x%02X -> %s", addr, esp_err_to_name(ok));
    }

    i2c_master_dev_handle_t dev;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    constexpr i2c_device_config_t devCfg = {
            .device_address = static_cast<uint16_t>(addr),
            .scl_speed_hz = I2C_FREQ,
    };
#pragma GCC diagnostic pop

    if (i2c_master_bus_add_device(bus, &devCfg, &dev) != ESP_OK) {
        ESP_LOGE(CURRENT_SENSOR_TAG, "Failed to add temporary device for scan");
        return;
    }

    constexpr uint8_t reg = 0xFE;
    uint8_t rx[2];

    const esp_err_t e1 = i2c_master_transmit(dev, &reg, 1, 500);
    ESP_LOGI(CURRENT_SENSOR_TAG, "TX(reg=0x%02X) -> %s\n", reg, esp_err_to_name(e1));
    if (e1 == ESP_OK) {
        const esp_err_t e2 = i2c_master_receive(dev, rx, 2, 500);
        ESP_LOGI(CURRENT_SENSOR_TAG, "RX -> %s, data=0x%02X%02X\n", esp_err_to_name(e2), rx[0], rx[1]);
    }

    i2c_master_bus_rm_device(dev);
}

#endif // MAIN_INCLUDE_CURRENT_SENSOR_HPP
