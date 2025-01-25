/**
 * TMP117 temperature sensor module
 */

#include "temperature_sensor.h"

#include <math.h>
#include <stdint.h>
#include "cmsis_os.h"
#include "main.h"

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

#define ALERT_MODE_BIT_POSITION     4
#define MOD_BITS_POSITION           10

typedef struct
{
    float temperature;
    osMutexId_t temperature_mutex;
} temperature_handler_t;

typedef struct
{
	bool alarm;
	osMutexId_t alarm_mutex;
} alarm_handler_t;

typedef struct
{
    temperature_handler_t temperature_handler;
    alarm_handler_t alarm_handler;
    osThreadId_t task_handle;
} ts_handler_t;

static ts_handler_t ts_handler;

static HAL_StatusTypeDef send_command(uint8_t reg, uint16_t value);
static HAL_StatusTypeDef read_register(uint8_t reg, uint16_t *value);
static HAL_StatusTypeDef reset_flags(void);
static HAL_StatusTypeDef update_temperature(void);
static void handle_error(void);
static void temperature_task(void *argument);
static void temeprature_sensor_trigger_alarm(void);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == TEMPERATURE_SENSOR_INT_Pin)
    {
    	if(ts_handler.alarm_handler.alarm == false)
    	{
    		temeprature_sensor_trigger_alarm();
        	reset_flags();
    	}
    }
}

bool temperature_sensor_init(void)
{
    bool init_ok = false;
    bool task_ok = false;
    bool mutex_ok = false;

    ts_handler.temperature_handler.temperature = NAN;
    ts_handler.alarm_handler.alarm = false;

    ts_handler.temperature_handler.temperature_mutex = osMutexNew(NULL);
    if (ts_handler.temperature_handler.temperature_mutex != NULL)
    {
    	ts_handler.alarm_handler.alarm_mutex = osMutexNew(NULL);
    	if(ts_handler.alarm_handler.alarm_mutex != NULL)
    {
        mutex_ok = true;
    	}
    }

    uint16_t config = (TMP117_MODE_CONTINUOUS << 10);
    init_ok = (send_command(TMP117_CONFIGURATION_REGISTER, config) == HAL_OK);

    const osThreadAttr_t task_attributes =
    {
        .name = "TemperatureTask",
        .priority = osPriorityNormal,
        .stack_size = 128 * 4
    };

    ts_handler.task_handle = osThreadNew(temperature_task, NULL, &task_attributes);
    task_ok = (ts_handler.task_handle != NULL);

    return init_ok && task_ok && mutex_ok;
}

float temperature_sensor_get_temperature(void)
{
    float temperature = NAN;

    if (osMutexAcquire(ts_handler.temperature_handler.temperature_mutex, osWaitForever) == osOK)
    {
        temperature = ts_handler.temperature_handler.temperature;

        if(osOK != osMutexRelease(ts_handler.temperature_handler.temperature_mutex))
        {
        	handle_error();
        }
    }
    else
    {
    	handle_error();
    }

    return temperature;
}

HAL_StatusTypeDef temperature_sensor_set_alarm(float high_temperature, float low_temperature)
{
    uint16_t high_temperature_value = (uint16_t)(high_temperature / 0.0078125f);
    uint16_t low_temperature_value = (uint16_t)(low_temperature / 0.0078125f);

	uint16_t config;

    if (read_register(TMP117_CONFIGURATION_REGISTER, &config) != HAL_OK)
    {
        return HAL_ERROR;
    }

    config &= ~(1 << ALERT_MODE_BIT_POSITION);
    config &= ~(3 << MOD_BITS_POSITION);

    if (send_command(TMP117_CONFIGURATION_REGISTER, config) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (send_command(TMP117_HIGH_TEMPERATURE_REGISTER, high_temperature_value) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (send_command(TMP117_LOW_TEMPERATURE_REGISTER, low_temperature_value) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

bool temperature_sensor_is_alarm_triggered(void)
{
	bool result = true;

    if (osMutexAcquire(ts_handler.alarm_handler.alarm_mutex, osWaitForever) == osOK)
    {
        result = ts_handler.alarm_handler.alarm;

        if(osOK != osMutexRelease(ts_handler.alarm_handler.alarm_mutex))
        {
        	handle_error();
        }
    }
    else
    {
    	handle_error();
    }

	return result;
}

void temperature_sensor_clear_alarm(void)
{
    if (osMutexAcquire(ts_handler.alarm_handler.alarm_mutex, osWaitForever) == osOK)
    {
    	ts_handler.alarm_handler.alarm = false;

        if(osOK != osMutexRelease(ts_handler.alarm_handler.alarm_mutex))
        {
        	handle_error();
        }
    }
    else
    {
    	handle_error();
    }
}

static HAL_StatusTypeDef reset_flags(void)
{
	HAL_StatusTypeDef status = HAL_ERROR;
    uint16_t config = 0U;

    status = read_register(TMP117_CONFIGURATION_REGISTER, &config);

    return status;
}

static HAL_StatusTypeDef update_temperature(void)
{
    HAL_StatusTypeDef status;
    uint16_t raw_temp;
    float calculated_temp;

    status = read_register(TMP117_TEMPERATURE_RESULT_REGISTER, &raw_temp);

    if (status == HAL_OK)
    {
    calculated_temp = (float)((int16_t)raw_temp) * 0.0078125f;

        if (osMutexAcquire(ts_handler.temperature_handler.temperature_mutex, osWaitForever) == osOK)
    {
            ts_handler.temperature_handler.temperature = calculated_temp;

            if(osOK != osMutexRelease(ts_handler.temperature_handler.temperature_mutex))
        {
        	handle_error();
        }
    }
    else
    {
    	handle_error();
        }
    }

    return status;
}

static void temperature_task(void *argument)
{
    (void)argument;

    for(;;)
    {
        update_temperature();
        osDelay(100);
    }
}

static HAL_StatusTypeDef send_command(uint8_t reg, uint16_t value)
{
	HAL_StatusTypeDef status = HAL_ERROR;
    uint8_t data[2];

    data[0] = (value >> 8) & 0xFF;
    data[1] = value & 0xFF;

    status = HAL_I2C_Mem_Write(&I2C_HANDLE, TMP117_I2C_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, data, 2, HAL_MAX_DELAY);

    return status;
}

static HAL_StatusTypeDef read_register(uint8_t reg, uint16_t *value)
{
	HAL_StatusTypeDef status = HAL_ERROR;
    uint8_t data[2];

    status = HAL_I2C_Mem_Read(&I2C_HANDLE, TMP117_I2C_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, data, 2, HAL_MAX_DELAY);

    if (status == HAL_OK)
    {
    *value = (data[0] << 8) | data[1];
    }

    return status;
}

static void temeprature_sensor_trigger_alarm(void)
{
    if (osMutexAcquire(ts_handler.alarm_handler.alarm_mutex, osWaitForever) == osOK)
    {
    	ts_handler.alarm_handler.alarm = true;

        if(osOK != osMutexRelease(ts_handler.alarm_handler.alarm_mutex))
        {
        	handle_error();
        }
    }
    else
    {
    	handle_error();
}
}

static void handle_error(void)
{
	HAL_GPIO_WritePin(HEATER_ON_GPIO_Port, HEATER_ON_Pin, GPIO_PIN_RESET);
	__asm volatile("BKPT #0");
}

