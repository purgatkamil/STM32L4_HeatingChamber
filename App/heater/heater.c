#include "heater.h"
#include "cmsis_os.h"
#include "main.h"
#include <math.h>

#include "temperature_sensor.h"

#define KP 15.0f
#define KI 0.4f
#define KD 0.4f

#define DEFAULT_SETPOINT 50.0f
#define CYCLE_TIME_MS 1000
#define MIN_HEATER_TIME_MS 20
#define TEMPERATURE_TOLERANCE 0.0f //off
#define STIMULATION_TOLERANCE 0.0f //off

typedef struct
{
    float kp;
    float ki;
    float kd;

    float setpoint;
    float integral;
    float previous_error;
    float previous_temperature;
    float current_power;
} pid_parameters_t;

typedef enum
{
    HEATER_MODE_OFF,
    HEATER_MODE_ON,
    HEATER_MODE_KEEP,

    HEATER_MODE_NUMBER,
} heater_mode_t;

typedef struct
{
    pid_parameters_t pid_params;
    bool heater_state;
    heater_mode_t mode;
    osThreadId_t task_handle;
    osMutexId_t mutex;
} pid_handler_t;

pid_handler_t pid_handler =
{
    .pid_params.kp = KP,
    .pid_params.ki = KI,
    .pid_params.kd = KD,

    .pid_params.setpoint = DEFAULT_SETPOINT,
    .pid_params.integral = 0.0f,
    .pid_params.previous_error = 0.0f,
    .pid_params.current_power = 0.0f,
    .pid_params.previous_temperature = 0.0f,

    .heater_state = false,
    .mode = HEATER_MODE_OFF,
};

static void pid_task(void *argument);
static void apply_pid_output(float pid_output);
static float calculate_pid_output(float current_temperature);

bool heater_init(void)
{
    bool mutex_ok = false;
    bool task_ok = false;

    pid_handler.mutex = osMutexNew(NULL);
    if (pid_handler.mutex != NULL)
    {
        mutex_ok = true;

        const osThreadAttr_t task_attributes =
        {
            .name = "PIDTask",
            .priority = osPriorityAboveNormal,
            .stack_size = 256 * 4
        };

        pid_handler.task_handle = osThreadNew(pid_task, NULL, &task_attributes);
        task_ok = (pid_handler.task_handle != NULL);
    }

    return mutex_ok && task_ok;
}

static void pid_task(void *argument)
{
    (void)argument;

    const TickType_t cycle_time = pdMS_TO_TICKS(CYCLE_TIME_MS);

    for (;;)
    {
        float current_temperature = temperature_sensor_get_temperature();

        if (osMutexAcquire(pid_handler.mutex, osWaitForever) == osOK)
        {
            float output = calculate_pid_output(current_temperature);
            osMutexRelease(pid_handler.mutex);

            apply_pid_output(output);
        }

        vTaskDelay(cycle_time);
    }
}

static float calculate_pid_output(float current_temperature)
{
    float error = pid_handler.pid_params.setpoint - current_temperature;

    pid_handler.pid_params.integral += error * (CYCLE_TIME_MS / 1000.0f);
    if (pid_handler.pid_params.integral > 100.0f / pid_handler.pid_params.ki)
    {
        pid_handler.pid_params.integral = 100.0f / pid_handler.pid_params.ki;
    }
    else if (pid_handler.pid_params.integral < -50.0f / pid_handler.pid_params.ki)
    {
        pid_handler.pid_params.integral = -50.0f / pid_handler.pid_params.ki;
    }

    float derivative = (error - pid_handler.pid_params.previous_error) / (CYCLE_TIME_MS / 1000.0f);

    float output = pid_handler.pid_params.kp * error +
                   pid_handler.pid_params.ki * pid_handler.pid_params.integral +
                   pid_handler.pid_params.kd * derivative;

    if (output > 100.0f) output = 100.0f;
    if (output < 0.0f) output = 0.0f;

    pid_handler.pid_params.current_power = output;
    pid_handler.pid_params.previous_error = error;
    return output;
}

static void apply_pid_output(float pid_output)
{
    float current_temperature = temperature_sensor_get_temperature();

    float duty_cycle = pid_output / 100.0f;
    TickType_t on_time = (TickType_t)(duty_cycle * CYCLE_TIME_MS);
    TickType_t off_time = CYCLE_TIME_MS - on_time;

    if (on_time > 0 && on_time < MIN_HEATER_TIME_MS)
    {
        on_time = MIN_HEATER_TIME_MS;
        off_time = CYCLE_TIME_MS - on_time;
    }

    if (pid_output <= 0.0f)
    {
        heater_turn_off();
        vTaskDelay(CYCLE_TIME_MS);
        return;
    }

    heater_turn_on();
    vTaskDelay(on_time);

    heater_turn_off();
    vTaskDelay(off_time);

    pid_handler.pid_params.previous_temperature = current_temperature;
}

void heater_turn_on(void)
{
    HAL_GPIO_WritePin(HEATER_ON_GPIO_Port, HEATER_ON_Pin, GPIO_PIN_SET);
    pid_handler.heater_state = true;
}

void heater_turn_off(void)
{
    HAL_GPIO_WritePin(HEATER_ON_GPIO_Port, HEATER_ON_Pin, GPIO_PIN_RESET);
    pid_handler.heater_state = false;
}
