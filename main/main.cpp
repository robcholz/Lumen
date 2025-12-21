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

#include "include/buzzer.hpp"
#include "include/current_sensor.hpp"
#include "include/display.hpp"
#include "include/efuse.hpp"
#include "include/encoder.hpp"
#include "include/motion.hpp"
#include "include/out_control.hpp"

extern "C" void main_app_run(); // NOLINT

extern "C" void main_app_log(const int32_t level, const char* text) { // NOLINT
    if (level == 0) {
        ESP_LOGD("[lumen:main]", "%s", text);
    } else if (level == 1) {
        ESP_LOGI("[lumen:main]", "%s", text);
    } else if (level == 2) {
        ESP_LOGE("[lumen:main]", "%s", text);
    } else if (level == 3) {
        ESP_LOGW("[lumen:main]", "%s", text);
    } else if (level == 4) {
        ESP_LOGV("[lumen:main]", "%s", text);
    }
}

extern "C" void main_app_abort(const char* details) { // NOLINT
    esp_system_abort(details);
}

static SemaphoreHandle_t S_LOG_MUTEX = nullptr;

extern "C" void acquire_main_app_log_buffer_lock() { // NOLINT
    xSemaphoreTake(S_LOG_MUTEX, portMAX_DELAY);
}

extern "C" void release_main_app_log_buffer_lock() { // NOLINT
    xSemaphoreGive(S_LOG_MUTEX);
}

extern "C" void current_sensor_init() { // NOLINT
    currentSensorInit();
}

extern "C" void current_sensor_read_debug() { // NOLINT
    currentSensorReadDebug();
}

extern "C" void control_init() { // NOLINT
    controlInit();
}

extern "C" void control_turn_on() { // NOLINT
    controlTurnOn();
}

extern "C" void control_turn_off() { // NOLINT
    controlTurnOff();
}

extern "C" void buzzer_init() { // NOLINT
    buzzerInit();
}

extern "C" void efuse_init() { // NOLINT
    efuseInit();
}

extern "C" void buzzer_tone(uint32_t freq_hz, uint16_t duration_ms) { // NOLINT
    buzzerTone(freq_hz, duration_ms);
}

extern "C" void* encoder_init(uint32_t longPressActivationDuration) { // NOLINT
    return encoderInit(longPressActivationDuration);
}

extern "C" void display_init(vision_ui_action_t (*callback)()) { // NOLINT
    displayInit(callback);
}

extern "C" void display_measure_fps() { // NOLINT
    displayFrameRender();
}

extern "C" void motion_init() { // NOLINT
    motionInit();
}

extern "C" void motion_read_debug() { // NOLINT
    motionReadDebug();
}

extern "C" void delay(const uint32_t ms) { // NOLINT
    vTaskDelay(pdMS_TO_TICKS(ms));
}

extern "C" void app_main() { // NOLINT
    S_LOG_MUTEX = xSemaphoreCreateMutex();

    main_app_run();
}
