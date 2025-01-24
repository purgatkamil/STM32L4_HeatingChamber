#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "stm32l4xx_hal.h"

float temperature_sensor_get_temperature(void);
HAL_StatusTypeDef temperature_sensor_set_alarm(float high_temperature, float low_temperature);