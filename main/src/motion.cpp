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
#include "include/motion.hpp"

#include <atomic>
#include <chrono>
#include <cmath>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>

#include <kalman_filter.hpp>
#include <lsm6dso.hpp>
#include <madgwick_filter.hpp>

#include "include/i2c_bus.hpp"
#include "include/pins.hpp"

static constexpr auto MOTION_TAG = "[lumen:motion]";

namespace {
    using Imu = espp::Lsm6dso<espp::lsm6dso::Interface::I2C>;

    std::atomic<float> S_ACCEL_X{0.0f};
    std::atomic<float> S_ACCEL_Y{0.0f};
    std::atomic<float> S_ACCEL_Z{0.0f};

    std::atomic<float> S_ROLL{0.0f};
    std::atomic<float> S_PITCH{0.0f};
    std::atomic<float> S_YAW{0.0f};

    std::atomic<float> S_GYRO_X{0.0f};
    std::atomic<float> S_GYRO_Y{0.0f};
    std::atomic<float> S_GYRO_Z{0.0f};

    Imu* S_IMU = nullptr;
    i2c_master_dev_handle_t S_IMU_DEV = nullptr;

    constexpr int K_I2C_TIMEOUT_MS = 50;

    bool imuWrite(uint8_t, const uint8_t* data, const size_t length) {
        if (!S_IMU_DEV) {
            ESP_LOGE(MOTION_TAG, "IMU I2C dev handle missing (write)");
            return false;
        }
        esp_err_t err = i2c_master_transmit(S_IMU_DEV, data, length, K_I2C_TIMEOUT_MS);
        if (err != ESP_OK) {
            ESP_LOGE(MOTION_TAG, "IMU write failed: %s", esp_err_to_name(err));
            return false;
        }
        return true;
    }

    bool imuRead(uint8_t, uint8_t* data, const size_t length) {
        if (!S_IMU_DEV) {
            ESP_LOGE(MOTION_TAG, "IMU I2C dev handle missing (read)");
            return false;
        }

        if (const esp_err_t err = i2c_master_receive(S_IMU_DEV, data, length, K_I2C_TIMEOUT_MS); err != ESP_OK) {
            ESP_LOGE(MOTION_TAG, "IMU read failed: %s", esp_err_to_name(err));
            return false;
        }
        return true;
    }

    bool imuWriteThenRead(uint8_t, const uint8_t* wdata, const size_t wlen, uint8_t* rdata, const size_t rlen) {
        if (!S_IMU_DEV) {
            ESP_LOGE(MOTION_TAG, "IMU I2C dev handle missing (write_then_read)");
            return false;
        }
        if (const esp_err_t err = i2c_master_transmit_receive(S_IMU_DEV, wdata, wlen, rdata, rlen, K_I2C_TIMEOUT_MS);
            err != ESP_OK) {
            ESP_LOGE(MOTION_TAG, "IMU write_then_read failed: %s", esp_err_to_name(err));
            return false;
        }
        return true;
    }

    constexpr float K_ANGLE_NOISE = 0.001f;
    constexpr float K_RATE_NOISE = 0.1f;

    espp::KalmanFilter<2> S_KF;

    constexpr float K_BETA = 0.1f;
    espp::MadgwickFilter MADGWICK(K_BETA);

    constexpr float degToRad(const float d) {
        return d * static_cast<float>(M_PI / 180.0);
    }

    [[noreturn]]
    void motionTask(void*) {
        ESP_LOGI(MOTION_TAG, "motion task started");

        int64_t lastUs = esp_timer_get_time();

        while (true) {
            const int64_t nowUs = esp_timer_get_time();
            float dt = (nowUs - lastUs) / 1'000'000.0f;
            if (dt <= 0) {
                dt = 1e-3f;
            }
            lastUs = nowUs;

            if (std::error_code ec; !S_IMU->update(dt, ec)) {
                ESP_LOGW(MOTION_TAG, "IMU update failed: %s", ec.message().c_str());
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            auto accel = S_IMU->get_accelerometer();
            auto gyro = S_IMU->get_gyroscope(); // deg/s

            // Mounting fix: upside down (180° about X) -> flip Y/Z axes.
            accel.y *= -1.0f;
            accel.z *= -1.0f;
            gyro.y *= -1.0f;
            gyro.z *= -1.0f;

            static constexpr float kG = 9.80665f;
            static float accelScale = kG;
            if (const float norm = sqrtf(accel.x * accel.x + accel.y * accel.y + accel.z * accel.z); norm > 0.1f) {
                const float targetScale = kG / norm;
                accelScale = accelScale * 0.95f + targetScale * 0.05f;
            }
            accel.x *= accelScale;
            accel.y *= accelScale;
            accel.z *= accelScale;

            S_ACCEL_X.store(accel.x);
            S_ACCEL_Y.store(accel.y);
            S_ACCEL_Z.store(accel.z);

            const float accelRoll = atan2(accel.y, accel.z);
            const float accelPitch = atan2(-accel.x, sqrt(accel.y * accel.y + accel.z * accel.z));

            S_KF.predict({degToRad(gyro.x), degToRad(gyro.y)}, dt);
            S_KF.update({accelRoll, accelPitch});
            auto [roll, pitch] = S_KF.get_state();

            MADGWICK.update(dt, accel.x, accel.y, accel.z, degToRad(gyro.x), degToRad(gyro.y), degToRad(gyro.z));
            float mRollDeg, mPitchDeg, mYawDeg;
            MADGWICK.get_euler(mRollDeg, mPitchDeg, mYawDeg);
            const float yaw = degToRad(mYawDeg + 90);

            S_ROLL.store(roll);
            S_PITCH.store(pitch);
            S_YAW.store(yaw);

            S_GYRO_X.store(degToRad(gyro.x));
            S_GYRO_Y.store(degToRad(gyro.y));
            S_GYRO_Z.store(degToRad(gyro.z));

            vTaskDelay(pdMS_TO_TICKS(40)); // ~25 Hz
        }
    }

} // namespace


void motionInit() {
    if (S_IMU) {
        return;
    }

    auto bus = lumen::i2c::get_shared_bus_handle();
    if (!bus) {
        ESP_LOGE(MOTION_TAG, "shared I2C bus not initialized");
        return;
    }

    if (!S_IMU_DEV) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        constexpr i2c_device_config_t imuDevCfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = Imu::DEFAULT_I2C_ADDRESS,
                .scl_speed_hz = I2C_FREQ,
        };
#pragma GCC diagnostic pop

        if (esp_err_t err = i2c_master_bus_add_device(bus, &imuDevCfg, &S_IMU_DEV); err != ESP_OK) {
            ESP_LOGE(MOTION_TAG, "add IMU device failed: %s", esp_err_to_name(err));
            return;
        }
    }

    S_KF.set_process_noise(K_RATE_NOISE);
    S_KF.set_measurement_noise(K_ANGLE_NOISE);

    auto kalmanFn = [](const float dt, const Imu::Value& accel, const Imu::Value& gyro) {
        const float accelRoll = atan2(accel.y, accel.z);
        const float accelPitch = atan2(-accel.x, sqrt(accel.y * accel.y + accel.z * accel.z));

        S_KF.predict({degToRad(gyro.x), degToRad(gyro.y)}, dt);
        S_KF.update({accelRoll, accelPitch});

        auto [roll, pitch] = S_KF.get_state();
        Imu::Value ori{};
        ori.roll = roll;
        ori.pitch = pitch;
        ori.yaw = 0;
        return ori;
    };

    const Imu::Config cfg{
            .device_address = Imu::DEFAULT_I2C_ADDRESS,
            .write = imuWrite,
            .read = imuRead,
            .imu_config =
                    {
                            .accel_range = Imu::AccelRange::RANGE_2G,
                            .accel_odr = Imu::AccelODR::ODR_416_HZ,
                            .gyro_range = Imu::GyroRange::DPS_2000,
                            .gyro_odr = Imu::GyroODR::ODR_416_HZ,
                    },
            .orientation_filter = kalmanFn,
            .auto_init = true,
            .log_level = espp::Logger::Verbosity::INFO,
    };

    static Imu imu(cfg);
    imu.set_write_then_read(imuWriteThenRead);
    S_IMU = &imu;

    std::error_code ec;
    if (!imu.set_accelerometer_filter(0b001, Imu::AccelFilter::LOWPASS, ec)) {
        ESP_LOGE(MOTION_TAG, "set accel filter failed: %s", ec.message().c_str());
    }

    if (!imu.set_gyroscope_filter(0b001, false, Imu::GyroHPF::HPF_0_26_HZ, ec)) {
        ESP_LOGE(MOTION_TAG, "set gyro filter failed: %s", ec.message().c_str());
    }

    xTaskCreate(motionTask, "motion_task", 6 * 1024, nullptr, 3, nullptr);
}

Acceleration motionGetAcceleration() {
    return {S_ACCEL_X.load(), S_ACCEL_Y.load(), S_ACCEL_Z.load()};
}

Angle motionGetAngle() {
    return {
            S_YAW.load(),
            S_ROLL.load(),
            S_PITCH.load(),
    };
}

AngleVelocity motionGetVelocity() {
    return {
            S_GYRO_Z.load(),
            S_GYRO_X.load(),
            S_GYRO_Y.load(),
    };
}

MotionStatus motionGetStatus() {
    return {"LIVE", "25 Hz"};
}

void motionReadDebug() {
    auto [x, y, z] = motionGetAcceleration();
    auto [yaw, roll, pitch] = motionGetAngle();
    auto [yaw1, roll1, pitch1] = motionGetVelocity();

    ESP_LOGI(
            MOTION_TAG,
            "acc=({%.3f, %.3f, %.3f}) "
            "angle(rpy)=({%.3f, %.3f, %.3f}) "
            "vel(rpy)=({%.3f, %.3f, %.3f})",
            x,
            y,
            z,
            roll,
            pitch,
            yaw,
            roll,
            pitch,
            yaw1
    );
}
