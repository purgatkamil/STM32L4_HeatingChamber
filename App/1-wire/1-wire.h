#pragma once

#include "stm32l4xx_hal.h"
#define ONEWIRE_PORT GPIOA
#define ONEWIRE_PIN GPIO_PIN_1

void OneWire_Init(void);
uint8_t OneWire_Reset(void);
void OneWire_WriteBit(uint8_t bit);
uint8_t OneWire_ReadBit(void);
void OneWire_WriteByte(uint8_t byte);
uint8_t OneWire_ReadByte(void);
