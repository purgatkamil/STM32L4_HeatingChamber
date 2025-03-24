#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "rtc.h"

bool rtc_init(void);

bool rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);
bool rtc_set_date(uint8_t weekday, uint8_t day, uint8_t month, uint8_t year);

RTC_TimeTypeDef rtc_get_time_struct(void);
RTC_DateTypeDef rtc_get_date_struct(void);
