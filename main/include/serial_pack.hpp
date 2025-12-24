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

#ifndef MAIN_INCLUDE_SERIAL_PACK_HPP
#define MAIN_INCLUDE_SERIAL_PACK_HPP

#include <cstddef>
#include <cstdint>

using SerialPackHandler = void (*)(const uint8_t* data, size_t currentSize);

// Initialize USB Serial/TAG driver and internal handler table. Safe to call multiple times.
extern void serialPackInit();
// Start the FreeRTOS task that parses incoming serial packs.
extern void serialPackStart();
// Stop the parsing task (driver remains initialized).
extern void serialPackStop();

// Register or replace a handler for a given path. Call after init and before start.
extern void serialPackAttachHandler(const char* path, SerialPackHandler handler);

#endif // MAIN_INCLUDE_SERIAL_PACK_HPP
