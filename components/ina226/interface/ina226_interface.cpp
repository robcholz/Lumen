/*
 *    Copyright 2024 Ziv Low
 *    
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *    
 *        http://www.apache.org/licenses/LICENSE-2.0
 *    
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "ina226_interface.h"
#include <utility>
#include <string>
#include "esp_err.h"

#define I2C_TIMEOUT_MS 100

std::expected<void, std::runtime_error> INA226::I2C_Write(const Register Register, const uint16_t Value) {
    esp_err_t err;

    const uint8_t WriteBuffer[] = {
        static_cast<uint8_t>(Register),
        static_cast<uint8_t>(Value >> 8),
        static_cast<uint8_t>(Value & 0xFF),
    };

    if (xSemaphoreTake(Lock, portMAX_DELAY) != pdTRUE) {
        return std::unexpected(std::runtime_error("I2C write timeout: could not take mutex"));
    }

    err = i2c_master_transmit(i2c_dev_handle, WriteBuffer, sizeof(WriteBuffer), I2C_TIMEOUT_MS);
    xSemaphoreGive(Lock);

    switch (err) {
        case ESP_OK:
            return {};
        case ESP_ERR_INVALID_ARG:
            return std::unexpected(std::runtime_error("I2C write invalid arg"));
        case ESP_ERR_TIMEOUT:
            return std::unexpected(std::runtime_error("I2C write timeout: could not transmit"));
        default:
            return std::unexpected(std::runtime_error("I2C write unknown error. err = " + std::to_string(err)));
    }
}

std::expected<uint16_t, std::runtime_error> INA226::I2C_Read(const INA226::Register Register) {
    esp_err_t err;

    const auto WriteBuffer = Register;

    uint16_t ReadBuffer;

    if (xSemaphoreTake(Lock, portMAX_DELAY) != pdTRUE) {
        return std::unexpected(std::runtime_error("I2C read timeout: could not take mutex"));
    }

    err = i2c_master_transmit_receive(i2c_dev_handle, reinterpret_cast<const uint8_t *>(&WriteBuffer), sizeof(WriteBuffer), reinterpret_cast<uint8_t *>(&ReadBuffer), sizeof(ReadBuffer), I2C_TIMEOUT_MS);
    xSemaphoreGive(Lock);
    
    switch (err) {
        case ESP_OK:
            return (ReadBuffer << 8) | (ReadBuffer >> 8);
        case ESP_ERR_INVALID_ARG:
            return std::unexpected(std::runtime_error("I2C read invalid arg"));
        case ESP_ERR_TIMEOUT:
            return std::unexpected(std::runtime_error("I2C read timeout"));
        default:
            return std::unexpected(std::runtime_error("I2C read unknown error. err = " + std::to_string(err)));
    }
}

INA226::INA226(const gpio_num_t sda_io_num, const gpio_num_t scl_io_num, const uint16_t address, const uint32_t scl_frequency, const i2c_port_num_t i2c_port_num)
    : i2c_bus_config{
        .i2c_port = i2c_port_num,
        .sda_io_num = sda_io_num,
        .scl_io_num = scl_io_num,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags{
            .enable_internal_pullup = true,
        },
    },
    i2c_dev_cfg{
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = scl_frequency,
    } 
    
{
    esp_err_t err;

    err = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (err != ESP_OK)
        throw std::runtime_error("I2C bus initialization failed. err = " + std::to_string(err));

    err = i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_cfg, &i2c_dev_handle);
    if (err != ESP_OK)
        throw std::runtime_error("I2C add device failed. err = " + std::to_string(err));

    err = CreateMutex(Lock);
    if (err != ESP_OK)
        throw std::runtime_error("I2C mutex creation failed. err = " + std::to_string(err));

    auto start_driver = InitDriver();
    if (start_driver.has_value() == false)
        throw std::runtime_error(std::string("INA226 driver initialization failed. err = ") + start_driver.error().what());
}

INA226::INA226(i2c_master_bus_handle_t bus_handle, const uint16_t address, const uint32_t scl_frequency)
    : i2c_bus_handle(bus_handle),
    i2c_dev_cfg{
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = scl_frequency,
    }
{
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_cfg, &i2c_dev_handle);
    if (err != ESP_OK)
        throw std::runtime_error("I2C add device failed. err = " + std::to_string(err));

    err = CreateMutex(Lock);
    if (err != ESP_OK)
        throw std::runtime_error("I2C mutex creation failed. err = " + std::to_string(err));

    auto start_driver = InitDriver();
    if (start_driver.has_value() == false)
        throw std::runtime_error(std::string("INA226 driver initialization failed. err = ") + start_driver.error().what());
}

esp_err_t INA226::CreateMutex(SemaphoreHandle_t &mutex) {
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
