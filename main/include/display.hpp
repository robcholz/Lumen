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

#ifndef MAIN_INCLUDE_DISPLAY_HPP
#define MAIN_INCLUDE_DISPLAY_HPP

#include <vision_ui_lib.h>

#define LCD_H_RES 240
#define LCD_V_RES 240

extern void displayFrameRender();

extern void displayInit(vision_ui_action_t (*callback)());

extern void displayDriverExtensionRGBBitmapDraw(
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height,
        const uint16_t* colorData
);

extern void displayDriverExtensionRGBBitmapAlphaDraw(
        int16_t x,
        int16_t y,
        int16_t width,
        int16_t height,
        const uint16_t* colorData
);

extern void displayDriverExtensionPixelScale(uint16_t scale);


#endif // MAIN_INCLUDE_DISPLAY_HPP
