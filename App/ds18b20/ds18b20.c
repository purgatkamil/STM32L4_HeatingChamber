#include "ds18b20.h"

float DS18B20_GetTemperature(void) {
    uint8_t lsb, msb;
    int16_t temp;

    if (!OneWire_Reset()) return -1000;

    OneWire_WriteByte(DS18B20_CMD_SKIP_ROM);
    OneWire_WriteByte(DS18B20_CMD_CONVERT_T);
    HAL_Delay(750);

    OneWire_Reset();
    OneWire_WriteByte(DS18B20_CMD_SKIP_ROM);
    OneWire_WriteByte(DS18B20_CMD_READ_SCRATCHPAD);

    lsb = OneWire_ReadByte();
    msb = OneWire_ReadByte();

    temp = (msb << 8) | lsb;

    return (float)temp / 16.0;
}
