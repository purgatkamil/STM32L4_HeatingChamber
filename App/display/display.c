/**
 * LCD display module
 */

#include "display.h"
#include "temperature_sensor.h"
#include "lcd.h"
#include "cmsis_os.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

#define LCD_TASK_STACK_SIZE (254 * 8)
#define LCD_TASK_PRIORITY   osPriorityNormal

typedef struct {
    char label[32];
    uint16_t color;
    uint16_t x;
    uint16_t y;
} display_field_handler_t;

typedef struct {
    display_field_handler_t temperature_field;
    osThreadId_t task_handle;
} display_handler_t;

static display_handler_t display_handler;

static void display_task(void *argument);

bool display_init(void)
{
    bool task_ok = false;

    lcd_init_freertos();
    lcd_fill(BLACK);

    strncpy(display_handler.temperature_field.label, "Temperature:", sizeof(display_handler.temperature_field.label) - 1);
    display_handler.temperature_field.color = WHITE;
    display_handler.temperature_field.x = 10;
    display_handler.temperature_field.y = 10;

    const osThreadAttr_t task_attributes = {
        .name = "DisplayTask",
        .priority = LCD_TASK_PRIORITY,
        .stack_size = LCD_TASK_STACK_SIZE
    };

    display_handler.task_handle = osThreadNew(display_task, NULL, &task_attributes);
    task_ok = (display_handler.task_handle != NULL);

    return task_ok;
}

static void display_temperature()
{
	char buffer[64];
	sprintf(buffer, "%s %0.2f",display_handler.temperature_field.label, temperature_sensor_get_temperature());


    LCD_DisplayString(
        display_handler.temperature_field.x,
		display_handler.temperature_field.y,
        buffer,
		display_handler.temperature_field.color,
        LCD_FONT12
    );
}

static void display_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        display_temperature();

        if (lcd_get_dirty() && !lcd_is_busy())
        {
            if (osMutexAcquire(lcd_get_mutex(), osWaitForever) == osOK)
            {
                lcd_copy();
                lcd_clear_dirty();
                osMutexRelease(lcd_get_mutex());
            }
        }

        osDelay(200);
    }
}

