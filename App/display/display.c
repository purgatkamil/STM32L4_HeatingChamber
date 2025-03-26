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
#include <rtc.h>

#include "rtc_module.h"
#include "charts.h"

#define LCD_TASK_STACK_SIZE (254 * 24)
#define LCD_TASK_PRIORITY   osPriorityNormal

typedef struct {
    char label[32];
    uint16_t color;
    uint16_t x;
    uint16_t y;
} display_field_handler_t;

typedef struct {
    display_field_handler_t temperature_field;
    display_field_handler_t time_field;
    osThreadId_t task_handle;
} display_handler_t;

static display_handler_t display_handler;

static void display_task(void *argument);

bool display_init(void)
{
    bool task_ok = false;
    bool lcd_ok = false;

    lcd_ok = lcd_init();
    lcd_fill(BLACK);

    strncpy(display_handler.temperature_field.label, "Temperature:", sizeof(display_handler.temperature_field.label) - 1);
    display_handler.temperature_field.color = WHITE;
    display_handler.temperature_field.x = 10;
    display_handler.temperature_field.y = 10;

    strncpy(display_handler.time_field.label, "Time:", sizeof(display_handler.time_field.label) - 1);
    display_handler.time_field.color = WHITE;
    display_handler.time_field.x = 10;
    display_handler.time_field.y = 30;

    const osThreadAttr_t task_attributes = {
        .name = "DisplayTask",
        .priority = LCD_TASK_PRIORITY,
        .stack_size = LCD_TASK_STACK_SIZE
    };

    display_handler.task_handle = osThreadNew(display_task, NULL, &task_attributes);
    task_ok = (display_handler.task_handle != NULL);

    return task_ok && lcd_ok;
}

static void display_temperature()
{
	char buffer[64];
	sprintf(buffer, "%s %0.2f",display_handler.temperature_field.label, temperature_sensor_get_temperature());

    lcd_display_string(
        display_handler.temperature_field.x,
		display_handler.temperature_field.y,
        buffer,
		display_handler.temperature_field.color,
        LCD_FONT12
    );
}

static void display_time()
{
	char buffer[64];
	RTC_TimeTypeDef time = rtc_get_time_struct();
	sprintf(buffer, "%s %02d:%02d:%02d",display_handler.time_field.label, time.Hours, time.Minutes, time.Seconds);

    lcd_display_string(
        display_handler.time_field.x,
		display_handler.time_field.y,
        buffer,
		display_handler.time_field.color,
        LCD_FONT12
    );
}

static void display_task(void *argument)
{
    (void)argument;
    for (;;)
    {
    	lcd_fill(BLACK);
    	osDelay(50);
       // display_temperature();
        //display_time();
        float temp = temperature_sensor_get_temperature();
        charts_shift_buffers();
        charts_push_value(0, 50.0f);
        charts_push_value(1, temp);
        charts_draw();
        lcd_copy();
        osDelay(500);
    }
}

