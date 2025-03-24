#include "rtc_module.h"

#include <stdio.h>

#include "rtc.h"

RTC_TimeTypeDef time;
RTC_DateTypeDef date;

void rtc_reset_time()
{
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	sTime.Hours = 0;
	sTime.Minutes = 0;
	sTime.Seconds = 0;
	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sDate.Month = RTC_MONTH_MARCH;
	sDate.Date = 24;
	sDate.Year = 25;
	HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

}

void rtc_get_time(char* timer)
{
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN); // Zawsze wywołuj obie funkcje razem!

    uint8_t seconds = time.Seconds;
    uint8_t minutes = time.Minutes;
    uint8_t hours   = time.Hours;

    sprintf(timer, "%02d:%02d:%02d", hours, minutes, seconds);
}

