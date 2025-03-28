# 🌡️ STM32L476 Heating Chamber

A temperature-controlled chamber project based on the STM32L476 microcontroller. The system reads temperature, displays data on an LCD screen, and controls a heater using a PID algorithm.

---

## 📁 Directory Structure

```
App/
├── 1-wire/                 # 1-Wire communication (DS18B20)
├── display/                # LCD display handling
├── ds18b20/                # Temperature sensor driver
├── heater/                 # PID algorithm and heater control
├── lcd/                    # LCD interface
├── rtc/                    # Real-time clock
├── temperature_sensor/     # Temperature read abstraction

Other folders:
Core/, Drivers/, Middlewares/, Debug/
```

## 🛠️ Environment

- Microcontroller: **STM32L476RG**
- IDE: **STM32CubeIDE**
- RTOS: **FreeRTOS**
- Display: e.g., **ILI9341** (SPI)
- Sensor: **DS18B20**
- Heater: GPIO-controlled (e.g., MOSFET or relay)

---

## 🚀 Getting Started

1. Open the project in STM32CubeIDE.
2. Flash the firmware to the STM32L476.
3. Connect the LCD, DS18B20 sensor, and heater.
4. On startup: temperature and time will be shown on the LCD.