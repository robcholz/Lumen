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

#ifndef MAIN_INCLUDE_I2C_BUS_HPP
#define MAIN_INCLUDE_I2C_BUS_HPP

#include <driver/i2c_master.h>
#include <esp_err.h>

namespace lumen::i2c {

    esp_err_t init_shared_bus();

    i2c_master_bus_handle_t get_shared_bus_handle();

} // namespace lumen::i2c

#endif // MAIN_INCLUDE_I2C_BUS_HPP
