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

#include <freertos/FreeRTOS.h>

#include <u8g2.h>

#include <vision_ui_lib.h>

static u8g2_t* U8G2 = nullptr;

void vision_ui_driver_bind(void* driver) {
    U8G2 = static_cast<u8g2_t*>(driver);
}

static int8_t STR_TOP = 0;
static int8_t STR_BOTTOM = 0;
static vision_ui_font_t CURRENT;

void vision_ui_driver_font_set(const vision_ui_font_t font) {
    u8g2_SetFont(U8G2, static_cast<const uint8_t*>(font.font));
    CURRENT = font;
    STR_TOP = font.top_compensation;
    STR_BOTTOM = font.bottom_compensation;
}

vision_ui_font_t vision_ui_driver_font_get() {
    return CURRENT;
}

void vision_ui_driver_str_draw(const uint16_t x, const uint16_t y, const char* str) {
    u8g2_DrawStr(U8G2, x, y - STR_BOTTOM, str);
}

void vision_ui_driver_str_utf8_draw(const uint16_t x, const uint16_t y, const char* str) {
    u8g2_DrawUTF8(U8G2, x, y - STR_BOTTOM, str);
}

uint16_t vision_ui_driver_str_width_get(const char* str) {
    return u8g2_GetStrWidth(U8G2, str);
}

uint16_t vision_ui_driver_str_utf8_width_get(const char* str) {
    return u8g2_GetUTF8Width(U8G2, str);
}

uint16_t vision_ui_driver_str_height_get() {
    int16_t h = u8g2_GetMaxCharHeight(U8G2);
    if (h < 0) {
        h = 0;
    }
    return static_cast<uint16_t>(h) + STR_TOP;
}

void vision_ui_driver_pixel_draw(const uint16_t x, const uint16_t y) {
    u8g2_DrawPixel(U8G2, x, y);
}

void vision_ui_driver_circle_draw(const uint16_t x, const uint16_t y, const uint16_t r) {
    u8g2_DrawCircle(U8G2, x, y, r, U8G2_DRAW_ALL);
}

void vision_ui_driver_disc_draw(const uint16_t x, const uint16_t y, const uint16_t r) {
    u8g2_DrawDisc(U8G2, x, y, r, U8G2_DRAW_ALL);
}

void vision_ui_driver_box_r_draw(
        const uint16_t x,
        const uint16_t y,
        const uint16_t w,
        const uint16_t h,
        const uint16_t r
) {
    u8g2_DrawRBox(U8G2, x, y, w, h, r);
}

void vision_ui_driver_box_draw(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h) {
    u8g2_DrawBox(U8G2, x, y, w, h);
}

void vision_ui_driver_frame_draw(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h) {
    u8g2_DrawFrame(U8G2, x, y, w, h);
}

void vision_ui_driver_frame_r_draw(
        const uint16_t x,
        const uint16_t y,
        const uint16_t w,
        const uint16_t h,
        const uint16_t r
) {
    u8g2_DrawRFrame(U8G2, x, y, w, h, r);
}

void vision_ui_driver_line_h_draw(const uint16_t x, const uint16_t y, const uint16_t l) {
    u8g2_DrawHLine(U8G2, x, y, l);
}

void vision_ui_driver_line_v_draw(const uint16_t x, const uint16_t y, const uint16_t h) {
    u8g2_DrawVLine(U8G2, x, y, h);
}

void vision_ui_driver_line_draw(const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2) {
    u8g2_DrawLine(U8G2, x1, y1, x2, y2);
}

void vision_ui_driver_line_h_dotted_draw(const uint16_t x, const uint16_t y, const uint16_t l) {
    for (uint16_t i = 0; i < l; i += 2) {
        u8g2_DrawPixel(U8G2, x + i, y);
    }
}

void vision_ui_driver_line_v_dotted_draw(const uint16_t x, const uint16_t y, const uint16_t h) {
    for (uint16_t i = 0; i < h; i += 2) {
        u8g2_DrawPixel(U8G2, x, y + i);
    }
}

void vision_ui_driver_bmp_draw(
        const uint16_t x,
        const uint16_t y,
        const uint16_t w,
        const uint16_t h,
        const uint8_t* bitMap
) {
    u8g2_DrawXBM(U8G2, x, y, w, h, bitMap);
}

void vision_ui_driver_color_draw(const uint8_t color) {
    u8g2_SetDrawColor(U8G2, color);
}

void vision_ui_driver_font_mode_set(const uint8_t mode) {
    u8g2_SetFontMode(U8G2, mode ? 1 : 0);
}

void vision_ui_driver_font_direction_set(const uint8_t dir) {
    u8g2_SetFontDirection(U8G2, static_cast<uint8_t>(dir & 0x03));
}

void vision_ui_driver_clip_window_set(const int16_t x0, const int16_t y0, const int16_t x1, const int16_t y1) {
    u8g2_SetClipWindow(U8G2, x0, y0, x1, y1);
}

void vision_ui_driver_clip_window_reset() {
    u8g2_SetMaxClipWindow(U8G2);
}

void vision_ui_driver_buffer_clear() {
    u8g2_ClearBuffer(U8G2);
}
