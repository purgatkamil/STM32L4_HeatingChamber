#pragma once

#include "1-wire.h"

#define DS18B20_CMD_CONVERT_T  0x44
#define DS18B20_CMD_READ_SCRATCHPAD  0xBE
#define DS18B20_CMD_SKIP_ROM  0xCC

float DS18B20_GetTemperature(void);

