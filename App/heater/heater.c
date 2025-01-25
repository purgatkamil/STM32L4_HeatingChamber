#include "heater.h"

#include "main.h"

void heater_turn_on(void)
{
	HAL_GPIO_WritePin(HEATER_ON_GPIO_Port, HEATER_ON_Pin, GPIO_PIN_SET);
}

void heater_turn_off(void)
{
	HAL_GPIO_WritePin(HEATER_ON_GPIO_Port, HEATER_ON_Pin, GPIO_PIN_RESET);
}

