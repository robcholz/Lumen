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

#ifndef MAIN_INCLUDE_ENCODER_HPP
#define MAIN_INCLUDE_ENCODER_HPP

#include <atomic>
#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include <esp_timer.h>

#include "pins.hpp"

#define EC_SW_DEBOUNCE_US 5000 // 8ms
#define EC_COUNTS_PER_STEP 4 // 1 detent = 4 pulse

static constexpr auto ENCODER_TAG = "[lumen:encoder]";

enum class EncoderEventType : uint8_t { RotateCW = 0, RotateCCW = 1, ButtonClick = 2, ButtonPress = 3 };

static QueueHandle_t ENCODER_EVENT_QUEUE = nullptr;
static TaskHandle_t ENCODER_TASK_HANDLE = nullptr;
static esp_timer_handle_t ENCODER_PRESS_TIMER = nullptr;
static uint32_t EC_SW_PRESS_THRESHOLD_US = 0;

static volatile int64_t LAST_SW_US = 0;
static std::atomic_bool BUTTON_PRESS_REPORTED = false;

struct EncoderRawEvent {
    int pin;
};
static volatile EncoderRawEvent LAST_EVT;

static int ENCODER_COUNT = 0;
static uint8_t LAST_AB_STATE = 0;

static int ENCODER_SW_LAST = 1;

static void IRAM_ATTR ecIsr(void* arg) {
    const int pin = (reinterpret_cast<intptr_t>(arg));
    const int64_t now = esp_timer_get_time();

    if (pin == PIN_NUM_EC_SW) {
        if (now - LAST_SW_US < EC_SW_DEBOUNCE_US) {
            return;
        }
        LAST_SW_US = now;
    }

    LAST_EVT.pin = pin;

    BaseType_t hp = pdFALSE;
    vTaskNotifyGiveFromISR(ENCODER_TASK_HANDLE, &hp);
    if (hp) {
        portYIELD_FROM_ISR();
    }
}

static void encoderQueueSendOverwrite(QueueHandle_t q, const EncoderEventType& evt) {
    if (!q) {
        return;
    }

    if (xQueueSend(q, &evt, 0) == pdTRUE) {
        return;
    }

    EncoderEventType dummy;
    xQueueReceive(q, &dummy, 0);

    xQueueSend(q, &evt, 0);
}

static void encoderPressTimerCallback(void*) {
    BUTTON_PRESS_REPORTED = true;

    if (ENCODER_EVENT_QUEUE) {
        ESP_LOGD(ENCODER_TAG, "press");
        encoderQueueSendOverwrite(ENCODER_EVENT_QUEUE, EncoderEventType::ButtonPress);
    }
}

[[noreturn]]
static void ecTask(void*) {
    LAST_AB_STATE = (gpio_get_level(PIN_NUM_EC_A) << 1) | (gpio_get_level(PIN_NUM_EC_B) << 0);

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (const int pin = LAST_EVT.pin; pin == PIN_NUM_EC_A || pin == PIN_NUM_EC_B) {
            const uint8_t a = gpio_get_level(PIN_NUM_EC_A);
            const uint8_t b = gpio_get_level(PIN_NUM_EC_B);
            const uint8_t ab = (a << 1) | b;

            const uint8_t transition = (LAST_AB_STATE << 2) | ab;
            int8_t step = 0;

            switch (transition) {
                case 0b0001:
                case 0b0111:
                case 0b1110:
                case 0b1000:
                    step = +1;
                    break;

                case 0b0010:
                case 0b0100:
                case 0b1101:
                case 0b1011:
                    step = -1;
                    break;

                default:
                    step = 0;
                    break;
            }

            LAST_AB_STATE = ab;

            if (step != 0) {
                ENCODER_COUNT += step;

                if (ENCODER_COUNT >= EC_COUNTS_PER_STEP) {
                    ENCODER_COUNT -= EC_COUNTS_PER_STEP;

                    if (ENCODER_EVENT_QUEUE) {
                        ESP_LOGD(ENCODER_TAG, "cw");
                        encoderQueueSendOverwrite(ENCODER_EVENT_QUEUE, EncoderEventType::RotateCW);
                    }
                } else if (ENCODER_COUNT <= -EC_COUNTS_PER_STEP) {
                    ENCODER_COUNT += EC_COUNTS_PER_STEP;

                    if (ENCODER_EVENT_QUEUE) {
                        ESP_LOGD(ENCODER_TAG, "ccw");
                        encoderQueueSendOverwrite(ENCODER_EVENT_QUEUE, EncoderEventType::RotateCCW);
                    }
                }
            }

        } else if (pin == PIN_NUM_EC_SW) {
            const int sw = gpio_get_level(PIN_NUM_EC_SW);
            if (ENCODER_SW_LAST == 1 && sw == 0) {
                BUTTON_PRESS_REPORTED = false;
                if (ENCODER_PRESS_TIMER) {
                    esp_timer_stop(ENCODER_PRESS_TIMER);
                    esp_timer_start_once(ENCODER_PRESS_TIMER, EC_SW_PRESS_THRESHOLD_US);
                }
            } else if (ENCODER_SW_LAST == 0 && sw == 1) {
                if (ENCODER_PRESS_TIMER && esp_timer_is_active(ENCODER_PRESS_TIMER)) {
                    esp_timer_stop(ENCODER_PRESS_TIMER);
                }

                if (!BUTTON_PRESS_REPORTED && ENCODER_EVENT_QUEUE) {
                    ESP_LOGD(ENCODER_TAG, "click");
                    encoderQueueSendOverwrite(ENCODER_EVENT_QUEUE, EncoderEventType::ButtonClick);
                }
            }

            ENCODER_SW_LAST = sw;
        }
    }
}

inline QueueHandle_t encoderInit(const uint32_t longPressActivationDuration) {
    EC_SW_PRESS_THRESHOLD_US = longPressActivationDuration;
    constexpr gpio_config_t cfgA = {
            .pin_bit_mask = 1ULL << PIN_NUM_EC_A,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&cfgA);

    constexpr gpio_config_t cfgB = {
            .pin_bit_mask = 1ULL << PIN_NUM_EC_B,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&cfgB);

    constexpr gpio_config_t cfgSW = {
            .pin_bit_mask = 1ULL << PIN_NUM_EC_SW,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&cfgSW);

    ENCODER_EVENT_QUEUE = xQueueCreate(16, sizeof(EncoderEventType));

    constexpr esp_timer_create_args_t pressTimerArgs = {
            .callback = &encoderPressTimerCallback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "encoder_press",
            .skip_unhandled_events = true,
    };
    esp_timer_create(&pressTimerArgs, &ENCODER_PRESS_TIMER);

    xTaskCreate(ecTask, "ec_task", 2048, nullptr, 10, &ENCODER_TASK_HANDLE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_NUM_EC_A, ecIsr, reinterpret_cast<void*>(static_cast<intptr_t>(PIN_NUM_EC_A)));
    gpio_isr_handler_add(PIN_NUM_EC_B, ecIsr, reinterpret_cast<void*>(static_cast<intptr_t>(PIN_NUM_EC_B)));
    gpio_isr_handler_add(PIN_NUM_EC_SW, ecIsr, reinterpret_cast<void*>(static_cast<intptr_t>(PIN_NUM_EC_SW)));

    return ENCODER_EVENT_QUEUE;
}

#endif // MAIN_INCLUDE_ENCODER_HPP
