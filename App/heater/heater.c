#include "heater.h"
#include "cmsis_os.h"
#include "main.h"
#include <math.h>

#include "temperature_sensor.h"

// Parametry PID
// Parametry PID dla dużych różnic
#define KP_HIGH 20.0f
#define KI_HIGH 0.5f
#define KD_HIGH 0.8f

// Parametry PID dla średnich różnic
#define KP_MEDIUM 15.0f
#define KI_MEDIUM 0.25f
#define KD_MEDIUM 0.5f

// Parametry PID dla małych różnic
#define KP_LOW 15.0f
#define KI_LOW 0.4f
#define KD_LOW 0.4f

#define DEFAULT_SETPOINT 50.0f//40.0f
#define CYCLE_TIME_MS 1000      // Całkowity okres sterowania (1 sekund)
#define MIN_HEATER_TIME_MS 20   // Minimalny czas włączenia grzałki
#define TEMPERATURE_TOLERANCE 0.0f // Histereza: ±0.5°C wokół setpointu
#define STIMULATION_TOLERANCE 0.0f

typedef struct {
    float kp_high;         // Wzmocnienie proporcjonalne dla dużych różnic
    float ki_high;         // Wzmocnienie całkowe dla dużych różnic
    float kd_high;         // Wzmocnienie różniczkujące dla dużych różnic

    float kp_medium;       // Wzmocnienie proporcjonalne dla średnich różnic
    float ki_medium;       // Wzmocnienie całkowe dla średnich różnic
    float kd_medium;       // Wzmocnienie różniczkujące dla średnich różnic

    float kp_low;          // Wzmocnienie proporcjonalne dla małych różnic
    float ki_low;          // Wzmocnienie całkowe dla małych różnic
    float kd_low;          // Wzmocnienie różniczkujące dla małych różnic

    float setpoint;        // Zadana temperatura
    float integral;        // Człon całkujący
    float previous_error;  // Poprzedni błąd
    float previous_temperature; // Poprzednia zmierzona temperatura
    float current_power;   // Aktualna moc grzałki w %
} pid_parameters_t;

typedef enum
{
	HEATER_MODE_OFF,
	HEATER_MODE_LOW,
	HEATER_MODE_MEDIUM,
	HEATER_MODE_HIGH,
	HEATER_MODE_KEEP,

	HEATER_MODE_NUMBER,
}heater_mode_t;

typedef struct {
    pid_parameters_t pid_params;
    bool heater_state;
    heater_mode_t mode;
    osThreadId_t task_handle;
    osMutexId_t mutex;
} pid_handler_t;

// Statyczny handler dla kontrolera PID
static pid_handler_t pid_handler = {
    .pid_params.kp_high = KP_HIGH,
    .pid_params.ki_high = KI_HIGH,
    .pid_params.kd_high = KD_HIGH,

    .pid_params.kp_medium = KP_MEDIUM,
    .pid_params.ki_medium = KI_MEDIUM,
    .pid_params.kd_medium = KD_MEDIUM,

    .pid_params.kp_low = KP_LOW,
    .pid_params.ki_low = KI_LOW,
    .pid_params.kd_low = KD_LOW,

    .pid_params.setpoint = DEFAULT_SETPOINT,
    .pid_params.integral = 0.0f,
    .pid_params.previous_error = 0.0f,

	.pid_params.current_power = 0.0f,
	.pid_params.previous_temperature = 0.0f,
    .heater_state = false,
	.mode = HEATER_MODE_OFF,
};

// Deklaracje funkcji statycznych
static void pid_task(void *argument);
static void apply_pid_output(float pid_output);
static float calculate_pid_output(float current_temperature);

// Inicjalizacja kontrolera PID
bool heater_init(void) {
    bool mutex_ok = false;
    bool task_ok = false;

    pid_handler.mutex = osMutexNew(NULL);
    if (pid_handler.mutex != NULL) {
        mutex_ok = true;

        const osThreadAttr_t task_attributes = {
            .name = "PIDTask",
            .priority = osPriorityNormal,
            .stack_size = 256 * 4
        };

        pid_handler.task_handle = osThreadNew(pid_task, NULL, &task_attributes);
        task_ok = (pid_handler.task_handle != NULL);
    }

    return mutex_ok && task_ok;
}

static void pid_task(void *argument) {
    (void)argument;

    const TickType_t cycle_time = pdMS_TO_TICKS(CYCLE_TIME_MS);

    for (;;) {
        float current_temperature = temperature_sensor_get_temperature();
        float error = pid_handler.pid_params.setpoint - current_temperature;

        // Sprawdzenie, czy temperatura jest w tolerancji
        if (fabsf(error) <= TEMPERATURE_TOLERANCE) {
            // Temperatura osiągnięta – grzałka pozostaje wyłączona
            heater_turn_off();
            vTaskDelay(cycle_time);
            continue;
        }

        // Obliczenie wyniku PID i zastosowanie sterowania
        if (osMutexAcquire(pid_handler.mutex, osWaitForever) == osOK) {
            float output = calculate_pid_output(current_temperature);
            osMutexRelease(pid_handler.mutex);

            apply_pid_output(output);
        }

        vTaskDelay(cycle_time);
    }
}

static float calculate_pid_output(float current_temperature) {
    float error = pid_handler.pid_params.setpoint - current_temperature;

 /*   // Blokada: powyżej setpointu wyłącz grzałkę
    if (error <= 0.0f) {
        pid_handler.pid_params.integral = 0.0f; // Zerowanie członu całkującego
        pid_handler.pid_params.previous_error = error; // Aktualizacja poprzedniego błędu
        pid_handler.pid_params.current_power = 0.0f; // Moc grzałki wynosi 0%
        return 0.0f; // Wyjście PID
    }*/

    // Wybór parametrów w zależności od różnicy temperatur
    float kp, ki, kd;
    if (fabsf(error) > 20.0f)
    {
        kp = pid_handler.pid_params.kp_high;
        ki = pid_handler.pid_params.ki_high;
        kd = pid_handler.pid_params.kd_high;
        pid_handler.mode = HEATER_MODE_HIGH;
    }
    else if (fabsf(error) > 10.0f)
    {
        kp = pid_handler.pid_params.kp_medium;
        ki = pid_handler.pid_params.ki_medium;
        kd = pid_handler.pid_params.kd_medium;
        pid_handler.mode = HEATER_MODE_MEDIUM;
    }
    else
    {
        kp = pid_handler.pid_params.kp_low;
        ki = pid_handler.pid_params.ki_low;
        kd = pid_handler.pid_params.kd_low;
        pid_handler.mode = HEATER_MODE_LOW;
    }

    // Człon całkujący z ograniczeniem (anti-windup)
    pid_handler.pid_params.integral += error * (CYCLE_TIME_MS / 1000.0f);
    if (pid_handler.pid_params.integral > 100.0f / ki) {
        pid_handler.pid_params.integral = 100.0f / ki;
    } else if (pid_handler.pid_params.integral < 0.0f) {
        pid_handler.pid_params.integral = 0.0f;
    }

    // Człon różniczkujący
    float derivative = (error - pid_handler.pid_params.previous_error) / (CYCLE_TIME_MS / 1000.0f);

    // Obliczenie wyniku PID
    float output = kp * error + ki * pid_handler.pid_params.integral + kd * derivative;

    // Ograniczenie wyjścia
    if (output > 100.0f) output = 100.0f;
    if (output < 0.0f) output = 0.0f;

    // Aktualizacja mocy grzałki
    pid_handler.pid_params.current_power = output;

    pid_handler.pid_params.previous_error = error;
    return output;
}

static void apply_pid_output(float pid_output) {
    float current_temperature = temperature_sensor_get_temperature();

    // Sprawdź, czy temperatura przekracza setpoint
   /* if (current_temperature > pid_handler.pid_params.setpoint) {
        // Jeśli temperatura spada i jest poniżej tolerancji stymulacji
        if (current_temperature < pid_handler.pid_params.previous_temperature &&
            current_temperature <= pid_handler.pid_params.setpoint + STIMULATION_TOLERANCE) {
            pid_handler.mode = HEATER_MODE_KEEP;
            heater_turn_on();
            vTaskDelay(pdMS_TO_TICKS(50)); // Minimalny impuls
            heater_turn_off();
            vTaskDelay(CYCLE_TIME_MS - pdMS_TO_TICKS(MIN_HEATER_TIME_MS)); // Reszta cyklu
        } else {
            // Temperatura nadal rośnie – całkowicie wyłącz grzałkę
            heater_turn_off();
            vTaskDelay(CYCLE_TIME_MS);
        }

        // Zaktualizuj poprzednią temperaturę
        pid_handler.pid_params.previous_temperature = current_temperature;
        return;
    }*/

    // Standardowe działanie PID, gdy temperatura jest poniżej setpointu
    float duty_cycle = pid_output / 100.0f;
    TickType_t on_time = (TickType_t)(duty_cycle * CYCLE_TIME_MS);
    TickType_t off_time = CYCLE_TIME_MS - on_time;

    // Minimalny czas włączenia grzałki
    if (on_time > 0 && on_time < MIN_HEATER_TIME_MS) {
        on_time = MIN_HEATER_TIME_MS;
        off_time = CYCLE_TIME_MS - on_time; // Dostosowanie czasu wyłączenia
    }

    // Jeśli wynik PID wskazuje minimalną moc, pozostaw grzałkę wyłączoną
    if (pid_output <= 0.0f) {
        heater_turn_off();
        vTaskDelay(CYCLE_TIME_MS);
        return;
    }

    // Włącz grzałkę na czas proporcjonalny do wyniku PID
    heater_turn_on();
    vTaskDelay(on_time);

    // Wyłącz grzałkę na resztę okresu
    heater_turn_off();
    vTaskDelay(off_time);

    // Zaktualizuj poprzednią temperaturę
    pid_handler.pid_params.previous_temperature = current_temperature;
}



void heater_turn_on(void) {
    HAL_GPIO_WritePin(HEATER_ON_GPIO_Port, HEATER_ON_Pin, GPIO_PIN_SET);
    pid_handler.heater_state = true;
}

void heater_turn_off(void) {
    HAL_GPIO_WritePin(HEATER_ON_GPIO_Port, HEATER_ON_Pin, GPIO_PIN_RESET);
    pid_handler.heater_state = false;
}
