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
#include "include/efuse.hpp"

#include <atomic>

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "include/current_sensor.hpp"
#include "include/out_control.hpp"

static constexpr auto EFUSE_TAG = "[lumen:efuse]";
static TaskHandle_t EFUSE_TASK_HANDLE = nullptr;
static constexpr int64_t AUTO_FAULT_RECOVERY_MILLISECOND = 3000;

static int64_t nowMs() {
    return esp_timer_get_time() / 1000;
}

static std::atomic<bool> HAS_OCP{false};
static std::atomic<bool> HAS_OVP{false};
static std::atomic<bool> HAS_FAULT{false};

[[noreturn]]
static void efuseTask(void*) {
    constexpr TickType_t period = pdMS_TO_TICKS(50); // 20 Hz
    TickType_t lastWake = xTaskGetTickCount();

    bool faultLatched = false;
    int64_t faultSinceMs = 0;

    bool usbOn = false;

    auto setUsbOn = [&]() {
        if (!usbOn) {
            controlTurnOn();
            usbOn = true;
        }
    };

    auto setUsbOff = [&]() {
        if (usbOn) {
            controlTurnOff();
            usbOn = false;
        }
    };

    for (;;) {
        // user forced off is not a "fault"
        if (LUMEN_CONFIG_VALUES.turnOffUsb) {
            setUsbOff();

            HAS_OCP.store(false, std::memory_order_relaxed);
            HAS_OVP.store(false, std::memory_order_relaxed);
            HAS_FAULT.store(false, std::memory_order_relaxed);

            vTaskDelayUntil(&lastWake, period);
            continue;
        }

        bool ovpNow = false;
        bool ocpNow = false;

        if (LUMEN_CONFIG_VALUES.overvoltageAlert) {
            if (const auto voltage = currentSensorReadVoltage(); voltage > LUMEN_CONFIG_VALUES.overvoltageMV) {
                ovpNow = true;
            }
        }

        if (LUMEN_CONFIG_VALUES.overcurrentAlert) {
            if (const auto current = currentSensorReadCurrent(); current > LUMEN_CONFIG_VALUES.overcurrentMA) {
                ocpNow = true;
            }
        }

        const bool faultNow = ovpNow || ocpNow;

        // publish realtime reasons
        HAS_OVP.store(ovpNow, std::memory_order_relaxed);
        HAS_OCP.store(ocpNow, std::memory_order_relaxed);

        bool offByFault = false;

        if (faultNow) {
            if (!faultLatched) {
                faultLatched = true;
                faultSinceMs = nowMs();
            }
            setUsbOff();
            offByFault = true;
        } else {
            if (!faultLatched) {
                setUsbOn();
                offByFault = false;
            } else {
                // faultLatched == true means we're still in "fault handling" mode
                if (LUMEN_CONFIG_VALUES.enableAutoFaultRecovery) {
                    if (const int64_t elapsed = nowMs() - faultSinceMs; elapsed >= AUTO_FAULT_RECOVERY_MILLISECOND) {
                        faultLatched = false;
                        setUsbOn();
                        offByFault = false;
                    } else {
                        setUsbOff();
                        offByFault = true;
                    }
                } else {
                    setUsbOff();
                    offByFault = true;
                }
            }
        }

        // publish: "USB is OFF because of OVP/OCP handling"
        HAS_FAULT.store(offByFault, std::memory_order_relaxed);

        vTaskDelayUntil(&lastWake, period);
    }
}

void efuseInit() {
    xTaskCreate(efuseTask, "efuse_task", 2048, nullptr, 10, &EFUSE_TASK_HANDLE);
    ESP_LOGI(EFUSE_TAG, "efuse task started (20Hz, prio=10)");
}

extern bool efuseHasOCP() {
    return HAS_OCP.load(std::memory_order_relaxed);
}

extern bool efuseHasOVP() {
    return HAS_OVP.load(std::memory_order_relaxed);
}

extern bool efuseHasFault() {
    return HAS_FAULT.load(std::memory_order_relaxed);
}
