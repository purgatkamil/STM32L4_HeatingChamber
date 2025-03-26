#include "charts.h"
#include <string.h>
#include <math.h>

#define CHART_HEIGHT     60
#define CHART_MARGIN     5

#define CHART_BOTTOM     (LCD_HEIGHT - CHART_MARGIN)
#define CHART_TOP        (LCD_HEIGHT - CHART_MARGIN - CHART_HEIGHT)
#define CHART_WIDTH      160
#define CHART_LEFT       0
#define CHART_RIGHT      (CHART_LEFT + CHART_WIDTH)


static chart_handler_t chart_handler;

static void handle_error(void);

bool charts_init(void)
{
    chart_handler.var_count = 0;
    chart_handler.chart_mutex = osMutexNew(NULL);
    charts_add_variable("Setpoint", 0);
	charts_add_variable("Temperature", 1);
    return chart_handler.chart_mutex != NULL;
}

bool charts_add_variable(const char* name, uint16_t color)
{
    if (chart_handler.var_count >= MAX_PLOTTED_VARIABLES)
        return false;

    uint8_t id = chart_handler.var_count++;
    strncpy(chart_handler.variables[id].name, name, sizeof(chart_handler.variables[id].name));
    chart_handler.variables[id].color = color;

    for (int i = 0; i < CIRCULAR_BUFFER_SIZE; i++)
        chart_handler.variables[id].buffer[i] = NAN;

    return true;
}

bool charts_push_value(uint8_t var_id, float value)
{
    if (var_id >= chart_handler.var_count) return false;

    if (osMutexAcquire(chart_handler.chart_mutex, osWaitForever) == osOK)
    {
        chart_handler.variables[var_id].buffer[0] = value;
        osMutexRelease(chart_handler.chart_mutex);
        return true;
    }

    return false;
}

void charts_clear(void)
{
    if (osMutexAcquire(chart_handler.chart_mutex, osWaitForever) == osOK)
    {
        for (int v = 0; v < chart_handler.var_count; ++v)
            for (int i = 0; i < CIRCULAR_BUFFER_SIZE; ++i)
                chart_handler.variables[v].buffer[i] = NAN;

        osMutexRelease(chart_handler.chart_mutex);
    }
}

void charts_shift_buffers(void)
{
    for (int v = 0; v < chart_handler.var_count; v++)
    {
        memmove(&chart_handler.variables[v].buffer[1],
                &chart_handler.variables[v].buffer[0],
                (CIRCULAR_BUFFER_SIZE - 1) * sizeof(float));
    }
}

void charts_draw(void)
{
    if (osMutexAcquire(chart_handler.chart_mutex, osWaitForever) != osOK)
        return;

    // Wyczyść tylko obszar wykresu
    for (int y = CHART_TOP; y <= CHART_BOTTOM; y++)
    {
        for (int x = CHART_LEFT; x < CHART_RIGHT; x++)
        {
            lcd_put_pixel(x, y, BLACK);
        }
    }

    for (int v = 0; v < chart_handler.var_count; v++)
    {
        float* buffer = chart_handler.variables[v].buffer;
        uint16_t color = chart_handler.variables[v].color;

        for (int x = 0; x < CHART_WIDTH - 1; x++)
        {
            float val1 = buffer[x];
            float val2 = buffer[x + 1];

            if (!isnan(val1) && !isnan(val2))
            {
                int y1 = CHART_BOTTOM - (int)val1;
                int y2 = CHART_BOTTOM - (int)val2;

                if (y1 < CHART_TOP) y1 = CHART_TOP;
                if (y1 > CHART_BOTTOM) y1 = CHART_BOTTOM;
                if (y2 < CHART_TOP) y2 = CHART_TOP;
                if (y2 > CHART_BOTTOM) y2 = CHART_BOTTOM;

                lcd_put_pixel(CHART_RIGHT - x, y1, color);
                lcd_put_pixel(CHART_RIGHT - x - 1, y2, color);
            }
        }
    }

    osMutexRelease(chart_handler.chart_mutex);
}


static void handle_error(void)
{

}
