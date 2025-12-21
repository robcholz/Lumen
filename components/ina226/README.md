# INA226 component for ESP-IDF

[![Component Registry](https://components.espressif.com/components/zivlow/ina226/badge.svg)](https://components.espressif.com/components/zivlow/ina226) [![Example build](https://github.com/zivlow/ina226/actions/workflows/build_example.yml/badge.svg)](https://github.com/zivlow/ina226/actions/workflows/build_example.yml)

This library is an ESP-IDF component for the INA226 power monitor IC.

## Using component
```bash
idf.py add-dependency "zivlow/ina226"
```

## Example
```bash
idf.py create-project-from-example "zivlow/ina226:ina226-example"
```

```bash
cd ina226-example
idf.py flash monitor
```

## I2C pins
If using ESP32 DevKitC, the default I2C pins has been set to GPIO 21 (SDA) and GPIO 22 (SCL).
```cpp
// Use default I2C pins GPIO 21 (SDA) and GPIO 22 (SCL)
INA226 CurrentSensor;
```

If using other ESP32 product variants (e.g. ESP32s3), or if you want to select custom I2C pins, put your desired I2C pins as parameter arguments.
```cpp
// Custom I2C pins
INA226 CurrentSensor{SDA_GPIO_NUM, SCL_GPIO_NUM};
```

## INA226 address
The default constructor will use default I2C address `0x40`.

To set a different I2C address, add the desired address as the third parameter.
```cpp
// Custom INA226 address
INA226 CurrentSensor{SDA_GPIO_NUM, SCL_GPIO_NUM, INA226_CUSTOM_ADDRESS};
```

## API
Refer to [Documentation](https://zivlow.github.io/ina226/html/index.html).

## Environment
- ESP-IDF v5.2
- ESP32 DevKitC
