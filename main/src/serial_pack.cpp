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

#include "include/serial_pack.hpp"

#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/usb_serial_jtag.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>


constexpr char SERIAL_PACK_TAG[] = "[lumen:serial_pack]";
constexpr size_t K_MAX_HANDLERS = 2;
constexpr size_t K_MAX_PATH_LEN = 16;
constexpr size_t K_MAX_DATA_LEN = 1024 * 2;
constexpr int64_t K_RX_TIMEOUT_US = 3 * 1000 * 1000;

struct HandlerEntry {
    char path[K_MAX_PATH_LEN];
    SerialPackHandler handler;
};

HandlerEntry S_HANDLERS[K_MAX_HANDLERS] = {};
size_t S_HANDLER_COUNT = 0;

TaskHandle_t S_SERIAL_TASK = nullptr;
volatile bool S_RUNNING = false;
bool S_INITIALIZED = false;

void logUnhandledData(const char* path, const uint8_t* data, const size_t len, const bool truncated) {
    char hexPreview[3 * 16 + 1] = {};
    const size_t previewLen = len > 16 ? 16 : len;
    for (size_t i = 0; i < previewLen; ++i) {
        static constexpr char kHex[] = "0123456789ABCDEF";
        hexPreview[i * 3] = kHex[(data[i] >> 4) & 0x0F];
        hexPreview[i * 3 + 1] = kHex[data[i] & 0x0F];
        hexPreview[i * 3 + 2] = (i + 1 < previewLen) ? ' ' : '\0';
    }
    ESP_LOGW(
            SERIAL_PACK_TAG,
            "unhandled path '%s', size=%u, data=%s%s",
            path,
            static_cast<unsigned>(len),
            hexPreview,
            truncated ? " ..." : ""
    );
}

SerialPackHandler findHandler(const char* path) {
    for (size_t i = 0; i < S_HANDLER_COUNT; ++i) {
        if (strcmp(S_HANDLERS[i].path, path) == 0) {
            return S_HANDLERS[i].handler;
        }
    }
    return nullptr;
}

void resetState(char* path, size_t& pathLen, uint8_t* data, size_t& dataLen, bool& inData) {
    path[0] = '\0';
    pathLen = 0;
    dataLen = 0;
    inData = false;
}

void logUnhandledPath(const char* path, const uint8_t* data, const size_t len, const bool truncated) {
    if (len > 0) {
        logUnhandledData(path, data, len, truncated);
    } else {
        ESP_LOGW(SERIAL_PACK_TAG, "unhandled path '%s', size=0", path);
    }
}

[[noreturn]]
void serialPackTask(void*) {
    static char path[K_MAX_PATH_LEN] = {};
    size_t pathLen = 0;
    static uint8_t data[K_MAX_DATA_LEN] = {};
    size_t dataLen = 0;
    bool inData = false;
    bool discardUntilNewline = false;
    uint8_t sizeBytes[sizeof(uint32_t)] = {};
    size_t sizeIndex = 0;
    uint32_t remaining = 0;
    int64_t lastRxUs = esp_timer_get_time();

    resetState(path, pathLen, data, dataLen, inData);

    auto handleByte = [&](const uint8_t byte) {
        if (discardUntilNewline) {
            if (byte == '\n') {
                discardUntilNewline = false;
                resetState(path, pathLen, data, dataLen, inData);
            }
            return;
        }

        if (!inData) {
            if (byte == '\r') {
                return;
            }
            if (byte == '\n') {
                if (pathLen == 0) {
                    return;
                }
                path[pathLen] = '\0';
                inData = true;
                dataLen = 0;
                sizeIndex = 0;
                remaining = 0;
                ESP_LOGD(SERIAL_PACK_TAG, "path is %s", path);
                return;
            }

            if (byte == ' ') {
                ESP_LOGE(SERIAL_PACK_TAG, "invalid path: contains space");
                discardUntilNewline = true;
                return;
            }

            if (pathLen + 1 >= K_MAX_PATH_LEN) {
                ESP_LOGE(SERIAL_PACK_TAG, "path too long");
                discardUntilNewline = true;
                return;
            }

            path[pathLen++] = static_cast<char>(byte);
            return;
        }

        if (sizeIndex < sizeof(uint32_t)) {
            sizeBytes[sizeIndex++] = byte;
            if (sizeIndex < sizeof(uint32_t)) {
                return;
            }

            const uint32_t size = static_cast<uint32_t>(sizeBytes[0]) | (static_cast<uint32_t>(sizeBytes[1]) << 8U) |
                                  (static_cast<uint32_t>(sizeBytes[2]) << 16U) |
                                  (static_cast<uint32_t>(sizeBytes[3]) << 24U);
            remaining = size;

            if (remaining == 0) {
                if (const SerialPackHandler handler = findHandler(path)) {
                    handler(nullptr, 0);
                } else {
                    logUnhandledPath(path, nullptr, 0, false);
                }
                resetState(path, pathLen, data, dataLen, inData);
            }
            return;
        }

        data[dataLen++] = byte;
        if (remaining > 0) {
            --remaining;
        }

        if (dataLen >= K_MAX_DATA_LEN || remaining == 0) {
            if (const SerialPackHandler handler = findHandler(path)) {
                if (dataLen > 0) {
                    handler(data, dataLen);
                }
            } else {
                logUnhandledData(path, data, dataLen, remaining > 0);
            }
            dataLen = 0;
        }

        if (remaining == 0) {
            if (const SerialPackHandler handler = findHandler(path)) {
                handler(nullptr, 0);
            } else {
                logUnhandledPath(path, nullptr, 0, false);
            }
            resetState(path, pathLen, data, dataLen, inData);
        }
    };

    static uint8_t rx[128] = {};
    while (S_RUNNING) {
        const int read = usb_serial_jtag_read_bytes(rx, sizeof(rx), pdMS_TO_TICKS(20));
        if (!S_RUNNING) {
            break;
        }
        if (read <= 0) {
            if (inData && sizeIndex >= sizeof(uint32_t) && remaining > 0) {
                const int64_t now = esp_timer_get_time();
                if (now - lastRxUs > K_RX_TIMEOUT_US) {
                    ESP_LOGW(SERIAL_PACK_TAG, "rx timeout, aborting pack");
                    resetState(path, pathLen, data, dataLen, inData);
                    discardUntilNewline = false;
                    sizeIndex = 0;
                    remaining = 0;
                    lastRxUs = now;
                }
            }
            continue;
        }
        lastRxUs = esp_timer_get_time();
        for (int i = 0; i < read; ++i) {
            handleByte(rx[i]);
        }
    }

    S_SERIAL_TASK = nullptr;
    vTaskDelete(nullptr);
    while (true) {
    }
}


void serialPackInit() {
    if (S_INITIALIZED) {
        return;
    }

    usb_serial_jtag_driver_config_t cfg = {
            .tx_buffer_size = 1024,
            .rx_buffer_size = 1024 * 16,
    };
    if (const esp_err_t err = usb_serial_jtag_driver_install(&cfg); err != ESP_OK) {
        ESP_LOGE(SERIAL_PACK_TAG, "usb_serial_jtag_driver_install failed: %s", esp_err_to_name(err));
        return;
    }

    S_INITIALIZED = true;
}

void serialPackStart() {
    if (!S_INITIALIZED) {
        serialPackInit();
    }

    if (!S_INITIALIZED || S_RUNNING) {
        return;
    }

    S_RUNNING = true;
    xTaskCreate(serialPackTask, "serial_pack", 1024 * 2, nullptr, 6, &S_SERIAL_TASK);
}

void serialPackStop() {
    if (!S_RUNNING) {
        return;
    }

    S_RUNNING = false;
}

void serialPackAttachHandler(const char* path, const SerialPackHandler handler) {
    if (!path || !handler) {
        ESP_LOGE(SERIAL_PACK_TAG, "invalid handler registration");
        return;
    }

    for (size_t i = 0; i < S_HANDLER_COUNT; ++i) {
        if (strcmp(S_HANDLERS[i].path, path) == 0) {
            S_HANDLERS[i].handler = handler;
            return;
        }
    }

    if (S_HANDLER_COUNT >= K_MAX_HANDLERS) {
        ESP_LOGE(SERIAL_PACK_TAG, "handler table full");
        return;
    }

    ESP_LOGI(SERIAL_PACK_TAG, "handler %s is attached", path);

    std::strncpy(S_HANDLERS[S_HANDLER_COUNT].path, path, K_MAX_PATH_LEN - 1);
    S_HANDLERS[S_HANDLER_COUNT].path[K_MAX_PATH_LEN - 1] = '\0';
    S_HANDLERS[S_HANDLER_COUNT].handler = handler;
    ++S_HANDLER_COUNT;
}
