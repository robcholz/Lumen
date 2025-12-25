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
#include "include/display.hpp"

#include <algorithm>

#include <esp_log.h>
#include <esp_timer.h>

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>

#include <driver/spi_master.h>

#include <u8g2.h>

#include <vision_ui_lib.h>

#include "include/pins.hpp"

#define HW_TAG "[lumen:display_hw_driver]"

#define LCD_BPP 16 // RGB565
#define LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000) // 80 MHz is typical for ST7789
#define BK_LIGHT_ON_LEVEL 1
#define BK_LIGHT_OFF_LEVEL (!BK_LIGHT_ON_LEVEL)

// DMA block lines (must divide V_RES)
#define PARALLEL_LINES 128

u8g2_t U8G2;
uint8_t G_U8G2_BUF[LCD_H_RES * LCD_V_RES / 8];

static uint16_t* S_LINES[2] = {nullptr, nullptr};
static volatile bool S_BUF_BUSY[2] = {false, false};

static inline esp_lcd_panel_handle_t PANEL = nullptr;

static u8x8_display_info_t U8G2_DISPLAY_INFO = {
        /* chip_enable_level = */ 0,
        /* chip_disable_level = */ 1,
        /* post_chip_enable_wait_ns = */ 0,
        /* pre_chip_disable_wait_ns = */ 0,
        /* reset_pulse_width_ms = */ 0,
        /* post_reset_wait_ms = */ 0,
        /* sda_setup_time_ns = */ 0,
        /* sck_pulse_width_ns = */ 0,
        /* sck_clock_hz = */ 4000000UL,
        /* spi_mode = */ 0,
        /* i2c_bus_clock_100kHz = */ 0,
        /* data_setup_time_ns = */ 0,
        /* write_pulse_width_ns = */ 0,
        /* tile_width = */ LCD_H_RES / 8,
        /* tile_hight = */ LCD_V_RES / 8,
        /* default_x_offset = */ 0,
        /* flipmode_x_offset = */ 0,
        /* pixel_width = */ LCD_H_RES,
        /* pixel_height = */ LCD_V_RES
};

static uint8_t u8x8DLumenCb(u8x8_t* u8x8, const uint8_t msg, const uint8_t argInt, void* argPtr) {
    switch (msg) {
        case U8X8_MSG_DISPLAY_SETUP_MEMORY:
            u8x8_d_helper_display_setup_memory(u8x8, &U8G2_DISPLAY_INFO);
            return 1;
        case U8X8_MSG_DISPLAY_INIT:
            u8x8_d_helper_display_init(u8x8);
            return 1;
        default:
            break;
    }
    (void) argInt;
    (void) argPtr;
    return 1;
}

static bool onColorTransDone(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t*, void*) {
    static volatile int sNextDoneIdx = 0;

    const int idx = sNextDoneIdx;
    S_BUF_BUSY[idx] = false;
    sNextDoneIdx ^= 1;
    return false;
}

static constexpr uint16_t rgb565(const uint8_t r, const uint8_t g, const uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static constexpr uint16_t U8G2_COLOR_OFF = rgb565(0, 0, 0);
static constexpr uint16_t U8G2_COLOR_ON = rgb565(255, 255, 255);

static void displayPrepareRGBBuffers() {
    static constexpr int bufSize = sizeof(S_LINES) / sizeof(S_LINES[0]);
    constexpr int neededBuffers = (LCD_V_RES + PARALLEL_LINES - 1) / PARALLEL_LINES;
    constexpr int buffersToClear = std::min(bufSize, neededBuffers);
    for (int i = 0; i < buffersToClear; ++i) {
        if (!S_LINES[i]) {
            continue;
        }
        while (S_BUF_BUSY[i]) {
            taskYIELD();
        }
        std::fill_n(S_LINES[i], LCD_H_RES * PARALLEL_LINES, U8G2_COLOR_OFF);
    }
}

static bool DISPLAY_READY = false;

void displayFrameRender() {
    if (!DISPLAY_READY) {
        return;
    }
    const uint32_t start = esp_timer_get_time();

    vision_ui_driver_buffer_clear();
    displayPrepareRGBBuffers();
    vision_ui_step_render();

    const uint32_t flash = esp_timer_get_time();
    vision_ui_driver_buffer_send();

    const uint32_t end = esp_timer_get_time();
    const float elapsed = (end - start) / 1e6;
    const float flashElapsed = (end - flash) / 1e6;
    const float fps = 1.0F / elapsed;
    ESP_LOGD(
            HW_TAG,
            "Frame time: %.3f s  =>  %.1f FPS, total time: %.1f, flash time: %.1f",
            elapsed,
            fps,
            elapsed * 1000,
            flashElapsed * 1000
    );
}

void* allocator(const vision_alloc_op_t op, const size_t size, const size_t count, void* ptr) {
    static size_t total = 0;
    switch (op) {
        case VisionAllocMalloc:
            total += size;
            ESP_LOGD(HW_TAG, "malloc: size %u, total: %u \n", size, total);
            return malloc(size);
        case VisionAllocCalloc:
            ESP_LOGD(HW_TAG, "calloc: size %u, count %u\n", size, count);
            return calloc(count, size);
        case VisionAllocFree:
            ESP_LOGD(HW_TAG, "free: %p\n", ptr);
            free(ptr);
            return nullptr;
    }
    return nullptr;
}

static inline esp_lcd_panel_io_handle_t PANEL_IO = nullptr;

vision_ui_action_t (*UI_ACTION_CALLBACK)();

void displayInit(vision_ui_action_t (*callback)()) {
    UI_ACTION_CALLBACK = callback;

    constexpr gpio_config_t bk = {
            .pin_bit_mask = 1ULL << PIN_NUM_BK,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&bk));
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_BK, BK_LIGHT_OFF_LEVEL));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    constexpr spi_bus_config_t busConfig = {
            .mosi_io_num = PIN_NUM_MOSI,
            .miso_io_num = -1,
            .sclk_io_num = PIN_NUM_SCLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t) + 8,
            .flags = SPICOMMON_BUSFLAG_MASTER
    };
#pragma GCC diagnostic pop

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &busConfig, SPI_DMA_CH_AUTO));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    constexpr esp_lcd_panel_io_spi_config_t ioCfg = {
            .cs_gpio_num = PIN_NUM_CS,
            .dc_gpio_num = PIN_NUM_DC,
            .spi_mode = 0,
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,
            .trans_queue_depth = 3,
            .on_color_trans_done = onColorTransDone,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .flags =
                    {
                            .dc_low_on_data = 0,
                            .octal_mode = 0,
                    },
    };
#pragma GCC diagnostic pop
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &ioCfg, &PANEL_IO));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    constexpr esp_lcd_panel_dev_config_t panelCfg =
            {.reset_gpio_num = PIN_NUM_RST,
             .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
             .bits_per_pixel = LCD_BPP,
             .flags = {
                     .reset_active_high = 0,
             }};
#pragma GCC diagnostic pop

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(PANEL_IO, &panelCfg, &PANEL));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(PANEL));
    ESP_ERROR_CHECK(esp_lcd_panel_init(PANEL));

    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(PANEL, true));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(PANEL, true));

    // Allocate DMA buffers
    for (int i = 0; i < 2; i++) {
        S_LINES[i] = static_cast<uint16_t*>(
                heap_caps_malloc(LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)
        );
        assert(S_LINES[i] && "Failed to alloc DMA buffer");
    }

    u8g2_SetupDisplay(&U8G2, u8x8DLumenCb, u8x8_cad_empty, u8x8_byte_empty, u8x8_dummy_cb);
    u8g2_SetupBuffer(&U8G2, G_U8G2_BUF, LCD_V_RES / 8, u8g2_ll_hvline_vertical_top_lsb, &u8g2_cb_r0);
    u8g2_InitDisplay(&U8G2);
    u8g2_SetPowerSave(&U8G2, 0);
    u8g2_ClearBuffer(&U8G2);

    // Turn on backlight
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_BK, BK_LIGHT_ON_LEVEL));

    vision_ui_driver_bind(&U8G2);
    vision_ui_allocator_set(allocator);

    lumenLoadLayout();
    DISPLAY_READY = true;
}

static uint16_t MONO_TO_RGB565[256][8];
static bool LUT_READY = false;

static void ensureMonoLut() {
    if (LUT_READY) {
        return;
    }
    for (int byteVal = 0; byteVal < 256; ++byteVal) {
        for (int bit = 0; bit < 8; ++bit) {
            MONO_TO_RGB565[byteVal][bit] = (byteVal & (1 << bit)) ? U8G2_COLOR_ON : U8G2_COLOR_OFF;
        }
    }
    LUT_READY = true;
}

void vision_ui_driver_buffer_send() {
    assert(S_LINES[0] && S_LINES[1]);
    ensureMonoLut();
    const uint8_t* mono = u8g2_GetBufferPtr(&U8G2);
    const uint8_t tileWidth = u8g2_GetBufferTileWidth(&U8G2);

    // Convert and push in PARALLEL_LINES blocks; apply 180° rotation here.
    int bufIdx = 0;
    for (int startY = 0; startY < LCD_V_RES; startY += PARALLEL_LINES) {
        const int linesThisBlock = std::min(PARALLEL_LINES, LCD_V_RES - startY);

        // Wait if this buffer is still in flight.
        while (S_BUF_BUSY[bufIdx]) {
            taskYIELD();
        }

        uint16_t* block = S_LINES[bufIdx];

        for (int line = 0; line < linesThisBlock; ++line) {
            const int dstY = startY + line;
            const int srcY = (LCD_V_RES - 1) - dstY;
            const uint8_t bitMask = static_cast<uint8_t>(1U << (srcY & 7));
            // u8g2 buffer stores 8-pixel tiles; stride is tileWidth * 8 bytes per tile row.
            const uint8_t* rowPtr = mono + (srcY / 8) * tileWidth * 8;
            uint16_t* row = block + line * LCD_H_RES;

            for (int dstX = 0; dstX < LCD_H_RES; ++dstX) {
                // row[dstX] = (rowPtr[srcX] & bitMask) ? U8G2_COLOR_ON : U8G2_COLOR_OFF;
                if (const int srcX = (LCD_H_RES - 1) - dstX; rowPtr[srcX] & bitMask) {
                    row[dstX] = U8G2_COLOR_ON;
                }
            }
        }

        S_BUF_BUSY[bufIdx] = true;
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(PANEL, 0, startY, LCD_H_RES, startY + linesThisBlock, block));
        bufIdx ^= 1;
    }
}

static uint16_t S_PIXEL_SCALE = 1;

void displayDriverExtensionRGBBitmapDraw(
        const int16_t x,
        const int16_t y,
        const int16_t width,
        const int16_t height,
        const uint16_t* colorData
) {
    if (!colorData || width <= 0 || height <= 0) {
        return;
    }

    const int scale = std::max<int>(1, S_PIXEL_SCALE);
    if (scale == 1) {
        const int16_t x0 = std::max<int16_t>(0, x);
        const int16_t y0 = std::max<int16_t>(0, y);
        const int16_t x1 = std::min<int16_t>(LCD_H_RES, x + width);
        const int16_t y1 = std::min<int16_t>(LCD_V_RES, y + height);
        if (x0 >= x1 || y0 >= y1) {
            return;
        }

        static constexpr int bufSize = sizeof(S_LINES) / sizeof(S_LINES[0]);
        bool waited[bufSize] = {false};

        for (int srcY = y0; srcY < y1; ++srcY) {
            const int inY = srcY - y;
            const int dstY = (LCD_V_RES - 1) - srcY;
            const int bufIdx = dstY / PARALLEL_LINES;
            if (bufIdx < 0 || bufIdx >= bufSize || !S_LINES[bufIdx]) {
                continue;
            }
            if (!waited[bufIdx]) {
                while (S_BUF_BUSY[bufIdx]) {
                    taskYIELD();
                }
                waited[bufIdx] = true;
            }

            const int bufYStart = bufIdx * PARALLEL_LINES;
            const int rowOffset = (dstY - bufYStart) * LCD_H_RES;

            for (int srcX = x0; srcX < x1; ++srcX) {
                const int inX = srcX - x;
                const int dstX = (LCD_H_RES - 1) - srcX;
                const uint16_t pixel = colorData[inY * width + inX];
                S_LINES[bufIdx][rowOffset + dstX] = pixel;
            }
        }
        return;
    }

    const int32_t scaledX = static_cast<int32_t>(x) * scale;
    const int32_t scaledY = static_cast<int32_t>(y) * scale;
    const int32_t scaledW = static_cast<int32_t>(width) * scale;
    const int32_t scaledH = static_cast<int32_t>(height) * scale;

    const int32_t x0 = std::max<int32_t>(0, scaledX);
    const int32_t y0 = std::max<int32_t>(0, scaledY);
    const int32_t x1 = std::min<int32_t>(LCD_H_RES, scaledX + scaledW);
    const int32_t y1 = std::min<int32_t>(LCD_V_RES, scaledY + scaledH);
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    static constexpr int bufSize = sizeof(S_LINES) / sizeof(S_LINES[0]);
    bool waited[bufSize] = {false};

    for (int32_t dstY = y0; dstY < y1; ++dstY) {
        const int inY = static_cast<int>((dstY - scaledY) / scale);
        const int dstRow = static_cast<int>(dstY);
        const int rotatedY = (LCD_V_RES - 1) - dstRow;
        const int bufIdx = rotatedY / PARALLEL_LINES;
        if (bufIdx < 0 || bufIdx >= bufSize || !S_LINES[bufIdx]) {
            continue;
        }
        if (!waited[bufIdx]) {
            while (S_BUF_BUSY[bufIdx]) {
                taskYIELD();
            }
            waited[bufIdx] = true;
        }

        const int bufYStart = bufIdx * PARALLEL_LINES;
        const int rowOffset = (rotatedY - bufYStart) * LCD_H_RES;

        for (int32_t dstX = x0; dstX < x1; ++dstX) {
            const int inX = static_cast<int>((dstX - scaledX) / scale);
            const int rotatedX = (LCD_H_RES - 1) - static_cast<int>(dstX);
            const uint16_t pixel = colorData[inY * width + inX];
            S_LINES[bufIdx][rowOffset + rotatedX] = pixel;
        }
    }
}

extern void displayDriverExtensionRGBBitmapAlphaDraw(
        const int16_t x,
        const int16_t y,
        const int16_t width,
        const int16_t height,
        const uint16_t* colorData
) {
    if (!colorData || width <= 0 || height <= 0) {
        return;
    }

    const int scale = std::max<int>(1, S_PIXEL_SCALE);
    if (scale == 1) {
        const int16_t x0 = std::max<int16_t>(0, x);
        const int16_t y0 = std::max<int16_t>(0, y);
        const int16_t x1 = std::min<int16_t>(LCD_H_RES, x + width);
        const int16_t y1 = std::min<int16_t>(LCD_V_RES, y + height);
        if (x0 >= x1 || y0 >= y1) {
            return;
        }

        static constexpr int bufSize = sizeof(S_LINES) / sizeof(S_LINES[0]);
        bool waited[bufSize] = {false};

        for (int srcY = y0; srcY < y1; ++srcY) {
            const int inY = srcY - y;
            const int dstY = (LCD_V_RES - 1) - srcY;
            const int bufIdx = dstY / PARALLEL_LINES;
            if (bufIdx < 0 || bufIdx >= bufSize || !S_LINES[bufIdx]) {
                continue;
            }
            if (!waited[bufIdx]) {
                while (S_BUF_BUSY[bufIdx]) {
                    taskYIELD();
                }
                waited[bufIdx] = true;
            }

            const int bufYStart = bufIdx * PARALLEL_LINES;
            const int rowOffset = (dstY - bufYStart) * LCD_H_RES;

            for (int srcX = x0; srcX < x1; ++srcX) {
                const int inX = srcX - x;
                const int dstX = (LCD_H_RES - 1) - srcX;
                if (const uint16_t pixel = colorData[inY * width + inX]; pixel != U8G2_COLOR_OFF) {
                    S_LINES[bufIdx][rowOffset + dstX] = pixel;
                }
            }
        }
        return;
    }

    const int32_t scaledX = static_cast<int32_t>(x) * scale;
    const int32_t scaledY = static_cast<int32_t>(y) * scale;
    const int32_t scaledW = static_cast<int32_t>(width) * scale;
    const int32_t scaledH = static_cast<int32_t>(height) * scale;

    const int32_t x0 = std::max<int32_t>(0, scaledX);
    const int32_t y0 = std::max<int32_t>(0, scaledY);
    const int32_t x1 = std::min<int32_t>(LCD_H_RES, scaledX + scaledW);
    const int32_t y1 = std::min<int32_t>(LCD_V_RES, scaledY + scaledH);
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    static constexpr int bufSize = sizeof(S_LINES) / sizeof(S_LINES[0]);
    bool waited[bufSize] = {false};

    for (int32_t dstY = y0; dstY < y1; ++dstY) {
        const int inY = static_cast<int>((dstY - scaledY) / scale);
        const int dstRow = static_cast<int>(dstY);
        const int rotatedY = (LCD_V_RES - 1) - dstRow;
        const int bufIdx = rotatedY / PARALLEL_LINES;
        if (bufIdx < 0 || bufIdx >= bufSize || !S_LINES[bufIdx]) {
            continue;
        }
        if (!waited[bufIdx]) {
            while (S_BUF_BUSY[bufIdx]) {
                taskYIELD();
            }
            waited[bufIdx] = true;
        }

        const int bufYStart = bufIdx * PARALLEL_LINES;
        const int rowOffset = (rotatedY - bufYStart) * LCD_H_RES;

        for (int32_t dstX = x0; dstX < x1; ++dstX) {
            const int inX = static_cast<int>((dstX - scaledX) / scale);
            const int rotatedX = (LCD_H_RES - 1) - static_cast<int>(dstX);
            if (const uint16_t pixel = colorData[inY * width + inX]; pixel != U8G2_COLOR_OFF) {
                S_LINES[bufIdx][rowOffset + rotatedX] = pixel;
            }
        }
    }
}

void displayDriverExtensionPixelScale(const uint16_t scale) {
    S_PIXEL_SCALE = scale > 0 ? scale : 1;
}

void* vision_ui_driver_buffer_pointer_get() {
    return G_U8G2_BUF;
}

void vision_ui_driver_buffer_area_send(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h) {
}

vision_ui_action_t vision_ui_driver_action_get() {
    return UI_ACTION_CALLBACK();
}

uint32_t vision_ui_driver_ticks_ms_get() {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void vision_ui_driver_delay(const uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}
