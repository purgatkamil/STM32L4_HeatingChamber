#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lcd.h"
#include "cmsis_os.h"

#define MAX_PLOTTED_VARIABLES 5
#define CIRCULAR_BUFFER_SIZE 160

typedef struct {
    float buffer[CIRCULAR_BUFFER_SIZE];
    uint16_t color;
    char name[16];
} chart_variable_t;

typedef struct {
    chart_variable_t variables[MAX_PLOTTED_VARIABLES];
    uint8_t var_count;
    osMutexId_t chart_mutex;
} chart_handler_t;

bool charts_init(void);
bool charts_add_variable(const char* name, uint16_t color);
bool charts_push_value(uint8_t var_id, float value);
void charts_shift_buffers(void);
void charts_clear(void);
void charts_draw(void);
