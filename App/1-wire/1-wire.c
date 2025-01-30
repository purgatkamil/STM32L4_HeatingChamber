#include "1-wire.h"
#include "tim.h"

void OneWire_Delay(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

void OneWire_SetPinOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = ONEWIRE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ONEWIRE_PORT, &GPIO_InitStruct);
}

void OneWire_SetPinInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = ONEWIRE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ONEWIRE_PORT, &GPIO_InitStruct);
}

uint8_t OneWire_Reset(void) {
    uint8_t presence = 0;

    OneWire_SetPinOutput();
    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
    OneWire_Delay(480);

    OneWire_SetPinInput();
    OneWire_Delay(70);

    presence = HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN) == GPIO_PIN_RESET ? 1 : 0;

    OneWire_Delay(410);
    return presence;
}

void OneWire_WriteBit(uint8_t bit) {
    OneWire_SetPinOutput();

    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
    OneWire_Delay(bit ? 10 : 60);

    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_SET);
    OneWire_Delay(bit ? 55 : 5);
}

uint8_t OneWire_ReadBit(void) {
    uint8_t bit = 0;

    OneWire_SetPinOutput();
    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
    OneWire_Delay(3);

    OneWire_SetPinInput();
    OneWire_Delay(10);

    bit = HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN);
    OneWire_Delay(50);

    return bit;
}

void OneWire_WriteByte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        OneWire_WriteBit(byte & 0x01);
        byte >>= 1;
    }
}

uint8_t OneWire_ReadByte(void) {
    uint8_t byte = 0;

    for (uint8_t i = 0; i < 8; i++) {
        byte >>= 1;
        if (OneWire_ReadBit()) {
            byte |= 0x80;
        }
    }
    return byte;
}
