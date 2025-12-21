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

#ifndef MAIN_INCLUDE_EFUSE_HPP
#define MAIN_INCLUDE_EFUSE_HPP

#include <cstdint>

struct LumenConfigValues {
    int16_t overcurrentMA;
    int16_t overvoltageMV;
    bool enableAutoFaultRecovery;
    bool turnOffUsb;
    bool overvoltageAlert;
    bool overcurrentAlert;
};

extern LumenConfigValues LUMEN_CONFIG_VALUES;

extern void efuseInit();

extern bool efuseHasOCP();

extern bool efuseHasOVP();

extern bool efuseHasFault();

#endif // MAIN_INCLUDE_EFUSE_HPP
