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

#ifndef MAIN_INCLUDE_MOTION_HPP
#define MAIN_INCLUDE_MOTION_HPP

struct Acceleration {
    float x;
    float y;
    float z;
};

struct Angle {
    float yaw;
    float roll;
    float pitch;
};

using AngleVelocity = Angle;

extern void motionInit();

// m/s^2
extern Acceleration motionGetAcceleration();

// rad
extern Angle motionGetAngle();

// rad/s
extern AngleVelocity motionGetVelocity();

struct MotionStatus {
    const char* stateText;
    const char* rateHzText;
};

extern MotionStatus motionGetStatus();

[[maybe_unused]]
extern void motionReadDebug();

#endif // MAIN_INCLUDE_MOTION_HPP
