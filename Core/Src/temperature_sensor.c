/**
 * TMP117 temperature sensor module
 */

#include "temperature_sensor.h"

#include <math.h>
#include <stdint.h>
#include "cmsis_os.h"

#define I2C_HANDLE hi2c1
extern I2C_HandleTypeDef I2C_HANDLE;

#define TMP117_I2C_ADDRESS (0x48 << 1)

#define TMP117_TEMPERATURE_RESULT_REGISTER   0x00
#define TMP117_CONFIGURATION_REGISTER        0x01
#define TMP117_HIGH_TEMPERATURE_REGISTER     0x02
#define TMP117_LOW_TEMPERATURE_REGISTER      0x03

#define TMP117_MODE_CONTINUOUS   0x00
#define TMP117_MODE_SHUTDOWN     0x01
#define TMP117_MODE_ONE_SHOT     0x03
