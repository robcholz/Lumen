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

#pragma once

#include <expected>
#include <stdexcept>

/**
 * @brief INA226 driver class for controlling and accessing the INA226 power monitor IC.
 * 
 * The INA226_Driver class provides a set of functions to initialize the INA226, read various measurements,
 * and configure its operating modes, averaging modes, conversion times, and alert triggers.
 * 
 * This class is designed to be inherited and implemented by a specific I2C driver for the target platform.
 * The I2C_Write and I2C_Read functions are abstract and need to be implemented by the derived class.
 * 
 * @note The INA226 is a power monitor IC that can measure shunt voltage, bus voltage, current, and power.
 * It also supports various operating modes, averaging modes, and conversion times.
 * 
 * @note The INA226_Driver class uses the std::expected type to handle errors and return values.
 * If an error occurs, a std::runtime_error is returned with an error message.
 * 
 * @note The INA226_Driver class is not thread-safe and should be used in a single-threaded environment.
 */
class INA226_Driver {
public:
    /**
     * @brief Operating modes of INA226.
     * 
     * Select continuous, triggered, or power-down mode of operation.
     * Defaults to continuous shunt and bus measurement mode.
     */
    enum class OperatingMode : uint8_t {
        POWER_DOWN = 0,                 ///< Power-down mode
        SHUNT_VOLTAGE_TRIGGERED,        ///< Shunt voltage triggered mode
        BUS_VOLTAGE_TRIGGERED,          ///< Bus voltage triggered mode
        SHUNT_AND_BUS_TRIGGERED,        ///< Shunt and bus voltage triggered mode
        ADC_OFF,                        ///< ADC off mode
        SHUNT_VOLTAGE_CONTINUOUS,       ///< Continuous shunt voltage measurement mode
        BUS_VOLTAGE_CONTINUOUS,         ///< Continuous bus voltage measurement mode
        SHUNT_AND_BUS_CONTINUOUS,       ///< Continuous shunt and bus voltage measurement mode
    };

    /**
     * @brief Averaging modes of INA226.
     * 
     * Select the number of samples to be averaged.
     * Defaults to 1 sample.
     */
    enum class AveragingMode : uint8_t {
        SAMPLE_1 = 0,                   ///< 1 sample
        SAMPLE_4,                       ///< 4 samples
        SAMPLE_16,                      ///< 16 samples
        SAMPLE_64,                      ///< 64 samples
        SAMPLE_128,                     ///< 128 samples
        SAMPLE_256,                     ///< 256 samples
        SAMPLE_512,                     ///< 512 samples
        SAMPLE_1024,                    ///< 1024 samples
    };

    /**
     * @brief Conversion times of INA226.
     * 
     * Select the conversion time for shunt and bus voltage measurements.
     * Defaults to 1100us (=1.1ms).
     */
    enum class ConversionTime : uint8_t {
        TIME_140_uS = 0,                ///< 140 microseconds
        TIME_204_uS,                    ///< 204 microseconds
        TIME_332_uS,                    ///< 332 microseconds
        TIME_588_uS,                    ///< 588 microseconds
        TIME_1100_uS,                   ///< 1100 microseconds
        TIME_2116_uS,                   ///< 2116 microseconds
        TIME_4156_uS,                   ///< 4156 microseconds
        TIME_8244_uS,                   ///< 8244 microseconds
    };

    /**
     * @brief Alert trigger masks of INA226.
     * 
     * Select the alert trigger masks.
     * Defaults to all disabled (Transparent mode).
     */
    enum class AlertTriggerMask : uint16_t {
        SHUNT_OVER_VOLTAGE = (1 << 15),         ///< Shunt over-voltage trigger mask
        SHUNT_UNDER_VOLTAGE = (1 << 14),        ///< Shunt under-voltage trigger mask
        BUS_OVER_VOLTAGE = (1 << 13),           ///< Bus over-voltage trigger mask
        BUS_UNDER_VOLTAGE = (1 << 12),          ///< Bus under-voltage trigger mask
        POWER_OVER_LIMIT = (1 << 11),            ///< Power over-limit trigger mask
        CONVERSION_READY = (1 << 10),           ///< Conversion ready trigger mask
        ALERT_FUNCTION_FLAG = (1 << 4),         ///< Alert function flag trigger mask
        CONVERSION_READY_FLAG = (1 << 3),       ///< Conversion ready flag trigger mask
        MATH_OVERFLOW_FLAG = (1 << 2),          ///< Math overflow flag trigger mask
        ALERT_POLARITY = (1 << 1),              ///< Alert polarity trigger mask
        ALERT_LATCH_ENABLE = (1 << 0),          ///< Alert latch enable trigger mask
        ERROR = 0,                             ///< Error trigger mask
    };

protected:
    enum class Const : uint16_t;
    enum class Register : uint8_t;
    enum class ConfigMask : uint16_t;
    enum class ConfigOffset : uint8_t;
    uint16_t Current_LSB_uA;            // uA/bit

public:
    /**
     * @brief Initializes the INA226 driver.
     * 
     * @param[in] ShuntResistor_mOhm Shunt resistor value in milli-Ohms. Defaults to 100mOhm.
     * @param[in] MaxCurrent_A Maximum current in Amps. Defaults to 1A.
     * @return std::expected<void, std::runtime_error> 
     */
    std::expected<void, std::runtime_error> InitDriver(const uint32_t ShuntResistor_mOhm = CONFIG_INA226_SHUNT_RESISTOR_MILLIOHMS, const uint32_t MaxCurrent_A = CONFIG_INA226_MAX_CURRENT_AMPS);

    /**
     * @name Getters
     * @{
     */

    /**
     * @brief Get the Shunt Voltage in micro-Volts.
     * 
     * @return int32_t 
     */
    int32_t GetShuntVoltage_uV();

    /**
     * @brief Get the Bus Voltage in milli-Volts.
     * 
     * @return int32_t 
     */
    int32_t GetBusVoltage_mV();
    int32_t GetBusVoltage_Raw();

    /**
     * @brief Get the Current in micro-Amps.
     * 
     * @return int32_t 
     */
    int32_t GetCurrent_uA();

    /**
     * @brief Get the Power in micro-Watts.
     * 
     * @return int32_t 
     */
    int32_t GetPower_uW();

    /**
     * @brief Get the configuration register value.
     * 
     * @return uint16_t 
     */
    uint16_t GetConfig();

    /**
     * @brief Get the manufacturer ID.
     * 
     * @return uint16_t 
     */
    uint16_t GetManufacturerID();

    /**
     * @brief Get the die ID.
     * 
     * @return uint16_t 
     */
    uint16_t GetDieID();

    /**
     * @brief Get the operating mode.
     * 
     * @return OperatingMode 
     */
    OperatingMode GetOperatingMode();

    /**
     * @brief Get the averaging mode.
     * 
     * @return AveragingMode 
     */
    AveragingMode GetAveragingMode();

    /**
     * @brief Get the bus voltage conversion time.
     * 
     * @return ConversionTime 
     */
    ConversionTime GetBusVoltageConversionTime();

    /**
     * @brief Get the shunt voltage conversion time.
     * 
     * @return ConversionTime 
     */
    ConversionTime GetShuntVoltageConversionTime();

    /**
     * @brief Get the alert trigger mask.
     * 
     * @return uint16_t 
     */
    uint16_t GetAlertTriggerMask();

    /**
     * @brief Get the alert limit value.
     * 
     * @return uint16_t 
     */
    uint16_t GetAlertLimitValue();

    /** @} */

    /**
     * @name Setters
     * @{
     */

    /**
     * @brief Reset the INA226.
     * 
     */
    void Reset();

    /**
     * @brief Calibrate the INA226.
     * 
     * @param[in] ShuntResistor_mOhm Shunt resistor value in milli-Ohms. Defaults to 100mOhm.
     * @param[in] MaxCurrent_A Maximum current in Amps. Defaults to 1A.
     * 
     * @note Max current is limited by the value of shunt resistor. 0.1ohm shunt can measure up to (2^15 -1)* 2.5µV / 0.1ohm = 819175µA (=0.819175A)
     * @note <https://github.com/esphome/issues/issues/2999#issuecomment-1038148627>
     */
    void Calibrate(const uint32_t ShuntResistor_mOhm = 100,  float MaxCurrent_A = 1);

    /**
     * @brief Set the configuration register value.
     * 
     * @param[in] Config The configuration register value.
     */
    void SetConfig(const uint16_t Config);

    /**
     * @brief Set the operating mode.
     * 
     * @param[in] Mode The operating mode.
     */
    void SetOperatingMode(OperatingMode Mode);

    /**
     * @brief Set the averaging mode.
     * 
     * @param[in] Mode The averaging mode.
     */
    void SetAveragingMode(AveragingMode Mode);

    /**
     * @brief Set the bus voltage conversion time.
     * 
     * @param[in] Time The bus voltage conversion time.
     */
    void SetBusVoltageConversionTime(ConversionTime Time);

    /**
     * @brief Set the shunt voltage conversion time.
     * 
     * @param[in] Time The shunt voltage conversion time.
     */
    void SetShuntVoltageConversionTime(ConversionTime Time);

    /**
     * @brief Set the alert trigger mask.
     * 
     * @param[in] AlertTriggerMask The alert trigger mask.
     * 
     * @note If multiple functions are enabled, the highest significant bit position Alert Function (D15-D11) takes priority and responds to the Alert Limit Register.
     */
    void SetAlertTriggerMask(AlertTriggerMask AlertTriggerMask);

    /**
     * @brief Set the alert limit value.
     * 
     * @param[in] AlertLimitValue The alert limit value.
     */
    void SetAlertLimitValue(uint16_t AlertLimitValue);

    /** @} */
protected:
    /**
     * @brief Write data to the specified register.
     * 
     * @param[in] Register The register to write to.
     * @param[in] Value The value to write.
     * @return std::expected<void, std::runtime_error> 
     */
    virtual std::expected<void, std::runtime_error> I2C_Write(const Register Register, const uint16_t Value) = 0;

    /**
     * @brief Read data from the specified register.
     * 
     * @param[in] Register The register to read from.
     * @return std::expected<uint16_t, std::runtime_error> 
     */
    virtual std::expected<uint16_t, std::runtime_error> I2C_Read(const Register Register) = 0;
};
