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

#include "ina226_driver.h"
#include <cmath>
#include <format>
#include <inttypes.h>
#include <utility>

enum class INA226_Driver::Const : uint16_t {
  BUS_VOLTAGE_LSB_uV = 1250,   // 1250 uV/bit
  SHUNT_VOLTAGE_LSB_nV = 2500, // 2500 nV/bit
  POWER_LSB_FACTOR = 25,
  CONFIG_RESET_VALUE = 0x4127,
};

enum class INA226_Driver::Register : uint8_t {
  CONFIGURATION = 0x00,
  SHUNT_VOLTAGE,
  BUS_VOLTAGE,
  POWER,
  CURRENT,
  CALIBRATION,
  MASK_ENABLE,
  ALERT_LIMIT,
  MANUFACTURER_ID = 0xFE,
  DIE_ID,
};

enum class INA226_Driver::ConfigMask : uint16_t {
  OPERATING_MODE = (1 << 0) | (1 << 1) | (1 << 2),
  SHUNT_VOLTAGE_CONVERSION_TIME = (1 << 3) | (1 << 4) | (1 << 5),
  BUS_VOLTAGE_CONVERSION_TIME = (1 << 6) | (1 << 7) | (1 << 8),
  AVERAGING_MODE = (1 << 9) | (1 << 10) | (1 << 11),
  RESET = (1 << 15),
};

enum class INA226_Driver::ConfigOffset : uint8_t {
  OPERATING_MODE = 0,
  SHUNT_VOLTAGE_CONVERSION_TIME = 3,
  BUS_VOLTAGE_CONVERSION_TIME = 6,
  AVERAGING_MODE = 9,
  RESET = 15,
};

std::expected<void, std::runtime_error>
INA226_Driver::InitDriver(const uint32_t ShuntResistor_mOhm,
                          const uint32_t MaxCurrent_A) {
  Reset();
  auto config = GetConfig();
  if (config == std::to_underlying(Const::CONFIG_RESET_VALUE)) {
    Calibrate(ShuntResistor_mOhm, MaxCurrent_A);
    return {};
  }
  return std::unexpected(std::runtime_error(
      std::format("INA226 Init fail: Reading configuration register {:x} is "
                  "different from expected {:x} upon reset",
                  config, std::to_underlying(Const::CONFIG_RESET_VALUE))));
}

int32_t INA226_Driver::GetShuntVoltage_uV() {
  auto result = I2C_Read(Register::SHUNT_VOLTAGE);
  if (result.has_value()) {
    return static_cast<int16_t>(result.value()) *
           std::to_underlying(Const::SHUNT_VOLTAGE_LSB_nV) / 1000;
  }
  return 0;
}

int32_t INA226_Driver::GetBusVoltage_mV() {
  auto result = I2C_Read(Register::BUS_VOLTAGE);
  if (result.has_value()) {
    return static_cast<int16_t>(result.value()) *
           std::to_underlying(Const::BUS_VOLTAGE_LSB_uV) / 1000;
  }
  return 0;
}

int32_t INA226_Driver::GetBusVoltage_Raw() {
    auto result = I2C_Read(Register::BUS_VOLTAGE);
    if (result.has_value()) {
        return static_cast<int16_t>(result.value());
    }
    return 0;
}

int32_t INA226_Driver::GetCurrent_uA() {
  auto result = I2C_Read(Register::CURRENT);
  if (result.has_value()) {
    return static_cast<int16_t>(result.value()) * Current_LSB_uA;
  }
  return 0;
}

int32_t INA226_Driver::GetPower_uW() {
  auto result = I2C_Read(Register::POWER);
  if (result.has_value()) {
    return static_cast<int16_t>(result.value()) *
           std::to_underlying(Const::POWER_LSB_FACTOR) * Current_LSB_uA;
  }
  return 0;
}

uint16_t INA226_Driver::GetConfig() {
  auto result = I2C_Read(Register::CONFIGURATION);
  if (result.has_value()) {
    return result.value();
  }
  return 0;
}

uint16_t INA226_Driver::GetManufacturerID() {
  auto result = I2C_Read(Register::MANUFACTURER_ID);
  if (result.has_value()) {
    return result.value();
  }
  return 0;
}

uint16_t INA226_Driver::GetDieID() {
  auto result = I2C_Read(Register::DIE_ID);
  if (result.has_value()) {
    return result.value();
  }
  return 0;
}

INA226_Driver::OperatingMode INA226_Driver::GetOperatingMode() {
  auto config = GetConfig();
  return static_cast<OperatingMode>(
      config & std::to_underlying(ConfigMask::OPERATING_MODE));
}

INA226_Driver::AveragingMode INA226_Driver::GetAveragingMode() {
  auto config = GetConfig();
  config &= std::to_underlying(ConfigMask::AVERAGING_MODE);
  config >>= std::to_underlying(ConfigOffset::AVERAGING_MODE);
  return static_cast<AveragingMode>(config);
}

INA226_Driver::ConversionTime INA226_Driver::GetBusVoltageConversionTime() {
  auto config = GetConfig();
  config &= std::to_underlying(ConfigMask::BUS_VOLTAGE_CONVERSION_TIME);
  config >>= std::to_underlying(ConfigOffset::BUS_VOLTAGE_CONVERSION_TIME);
  return static_cast<ConversionTime>(config);
}

INA226_Driver::ConversionTime INA226_Driver::GetShuntVoltageConversionTime() {
  auto config = GetConfig();
  config &= std::to_underlying(ConfigMask::SHUNT_VOLTAGE_CONVERSION_TIME);
  config >>= std::to_underlying(ConfigOffset::SHUNT_VOLTAGE_CONVERSION_TIME);
  return static_cast<ConversionTime>(config);
}

uint16_t INA226_Driver::GetAlertTriggerMask() {
  auto result = I2C_Read(Register::MASK_ENABLE);
  if (result.has_value()) {
    return result.value();
  }
  return 0;
}

uint16_t INA226_Driver::GetAlertLimitValue() {
  auto result = I2C_Read(Register::ALERT_LIMIT);
  if (result.has_value()) {
    return result.value();
  }
  return 0;
}

void INA226_Driver::Reset() {
  auto config = GetConfig();
  auto config_value = config | std::to_underlying(ConfigMask::RESET);
  SetConfig(config_value);
}

void INA226_Driver::Calibrate(const uint32_t ShuntResistor_mOhm,
                              const float MaxCurrent_A) {
  float current_lsb_A = MaxCurrent_A / 32768.0f;
  Current_LSB_uA = current_lsb_A * 1e6f;

  float Rsh_ohm = ShuntResistor_mOhm * 1e-3f;

  // Cal = 0.00512 / (Current_LSB * Rsh)
  float cal_f = 0.00512f / (current_lsb_A * Rsh_ohm);

  if (cal_f < 0.0f)
    cal_f = 0.0f;
  if (cal_f > 65535.0f)
    cal_f = 65535.0f;
  uint16_t cal = static_cast<uint16_t>(cal_f + 0.5f);

  I2C_Write(Register::CALIBRATION, cal);
}

void INA226_Driver::SetConfig(const uint16_t Config) {
  I2C_Write(Register::CONFIGURATION, Config);
}

void INA226_Driver::SetOperatingMode(OperatingMode Mode) {
  auto config = GetConfig();
  config &= ~std::to_underlying(ConfigMask::OPERATING_MODE);
  config |= std::to_underlying(Mode)
            << std::to_underlying(ConfigOffset::OPERATING_MODE);
  I2C_Write(Register::CONFIGURATION, config);
}

void INA226_Driver::SetAveragingMode(AveragingMode Mode) {
  auto config = GetConfig();
  config &= ~std::to_underlying(ConfigMask::AVERAGING_MODE);
  config |= std::to_underlying(Mode)
            << std::to_underlying(ConfigOffset::AVERAGING_MODE);
  I2C_Write(Register::CONFIGURATION, config);
}

void INA226_Driver::SetBusVoltageConversionTime(ConversionTime Time) {
  auto config = GetConfig();
  config &= ~std::to_underlying(ConfigMask::BUS_VOLTAGE_CONVERSION_TIME);
  config |= std::to_underlying(Time)
            << std::to_underlying(ConfigOffset::BUS_VOLTAGE_CONVERSION_TIME);
  I2C_Write(Register::CONFIGURATION, config);
}

void INA226_Driver::SetShuntVoltageConversionTime(ConversionTime Time) {
  auto config = GetConfig();
  config &= ~std::to_underlying(ConfigMask::SHUNT_VOLTAGE_CONVERSION_TIME);
  config |= std::to_underlying(Time)
            << std::to_underlying(ConfigOffset::SHUNT_VOLTAGE_CONVERSION_TIME);
  I2C_Write(Register::CONFIGURATION, config);
}

void INA226_Driver::SetAlertTriggerMask(AlertTriggerMask AlertTriggerMask) {
  I2C_Write(Register::MASK_ENABLE, std::to_underlying(AlertTriggerMask));
}

void INA226_Driver::SetAlertLimitValue(uint16_t AlertLimitValue) {
  I2C_Write(Register::ALERT_LIMIT, AlertLimitValue);
}
