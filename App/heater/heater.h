#pragma once
#include <stdbool.h>
#include "cmsis_os.h"

bool heater_init(void);
void heater_turn_on(void);
void heater_turn_off(void);
