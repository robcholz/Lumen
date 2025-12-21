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

#include <inttypes.h>
#include <utility>
#include <bitset>
#include "ina226_interface.h"
#include "esp_log.h"

static const char *const TAG = "ina226-example";

extern "C" void app_main(void)
{
    // Initialize current sensor
    INA226 CurrentSensor;

    // Configure current sensor
    CurrentSensor.Calibrate(100, 1);
    CurrentSensor.SetOperatingMode(INA226::OperatingMode::SHUNT_AND_BUS_CONTINUOUS);
    CurrentSensor.SetAveragingMode(INA226::AveragingMode::SAMPLE_1024);
    CurrentSensor.SetBusVoltageConversionTime(INA226::ConversionTime::TIME_8244_uS);
    CurrentSensor.SetShuntVoltageConversionTime(INA226::ConversionTime::TIME_8244_uS);

    while (true) {
        // Read from current sensor
        ESP_LOGI(TAG, "\n");
        ESP_LOGI(TAG, "Shunt voltage: %" PRIi32 " uV", CurrentSensor.GetShuntVoltage_uV());
        ESP_LOGI(TAG, "Bus voltage: %" PRIi32 " mV", CurrentSensor.GetBusVoltage_mV());
        ESP_LOGI(TAG, "Current: %" PRIi32 " uA", CurrentSensor.GetCurrent_uA());
        ESP_LOGI(TAG, "Power: %" PRIi32 " uW", CurrentSensor.GetPower_uW());
        ESP_LOGI(TAG, "Config: %" PRIx16, CurrentSensor.GetConfig());
        ESP_LOGI(TAG, "Manufacturer ID: %" PRIx16, CurrentSensor.GetManufacturerID());
        ESP_LOGI(TAG, "Die ID: %" PRIx16, CurrentSensor.GetDieID());
        ESP_LOGI(TAG, "Operating mode: %" PRIu8, std::to_underlying(CurrentSensor.GetOperatingMode()));
        ESP_LOGI(TAG, "Averaging mode: %" PRIu8, std::to_underlying(CurrentSensor.GetAveragingMode()));
        ESP_LOGI(TAG, "Bus voltage conversion time: %" PRIu8, std::to_underlying(CurrentSensor.GetBusVoltageConversionTime()));
        ESP_LOGI(TAG, "Shunt voltage conversion time: %" PRIu8, std::to_underlying(CurrentSensor.GetShuntVoltageConversionTime()));
        ESP_LOGI(TAG, "Alert trigger mask: 0b%s" , std::bitset<16>(CurrentSensor.GetAlertTriggerMask()).to_string().c_str());
        ESP_LOGI(TAG, "Alert limit value: %" PRIu16, CurrentSensor.GetAlertLimitValue());

        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
