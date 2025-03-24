#include "rtc_module.h"

#include "rtc_module.h"
#include "cmsis_os.h"

typedef struct
{
    RTC_TimeTypeDef current_time;
    RTC_DateTypeDef current_date;
    osMutexId_t mutex;
} rtc_handler_t;

static rtc_handler_t rtc_handler;
static osThreadId_t rtc_task_handle;

static void rtc_task(void *argument);
static bool rtc_reset_time_and_date(void);
static void handle_error(void);

bool rtc_init(void)
{
    bool mutex_initialized = false;
    bool task_initialized = false;
    bool rtc_initialized = false;

    rtc_handler.mutex = osMutexNew(NULL);
    mutex_initialized = (rtc_handler.mutex != NULL);

    const osThreadAttr_t rtc_task_attributes = {
        .name = "RTCTask",
        .priority = osPriorityLow,
        .stack_size = 128 * 4
    };

    rtc_task_handle = osThreadNew(rtc_task, NULL, &rtc_task_attributes);
    task_initialized = (rtc_task_handle != NULL);

    rtc_initialized = rtc_reset_time_and_date();

    return mutex_initialized && task_initialized && rtc_initialized;
}

bool rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    RTC_TimeTypeDef new_time = {0};
    new_time.Hours = hours;
    new_time.Minutes = minutes;
    new_time.Seconds = seconds;

    return HAL_RTC_SetTime(&hrtc, &new_time, RTC_FORMAT_BIN) == HAL_OK;
}

bool rtc_set_date(uint8_t weekday, uint8_t day, uint8_t month, uint8_t year)
{
    RTC_DateTypeDef new_date = {0};
    new_date.WeekDay = weekday;
    new_date.Date = day;
    new_date.Month = month;
    new_date.Year = year;

    return HAL_RTC_SetDate(&hrtc, &new_date, RTC_FORMAT_BIN) == HAL_OK;
}

RTC_TimeTypeDef rtc_get_time_struct(void)
{
    RTC_TimeTypeDef time_copy = {0};

    if (osMutexAcquire(rtc_handler.mutex, osWaitForever) == osOK)
    {
        time_copy = rtc_handler.current_time;

        if (osMutexRelease(rtc_handler.mutex) != osOK)
        {
            handle_error();
        }
    }
    else
    {
        handle_error();
    }

    return time_copy;
}

RTC_DateTypeDef rtc_get_date_struct(void)
{
    RTC_DateTypeDef date_copy = {0};

    if (osMutexAcquire(rtc_handler.mutex, osWaitForever) == osOK)
    {
        date_copy = rtc_handler.current_date;

        if (osMutexRelease(rtc_handler.mutex) != osOK)
        {
            handle_error();
        }
    }
    else
    {
        handle_error();
    }

    return date_copy;
}

static void rtc_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        RTC_TimeTypeDef time_buffer;
        RTC_DateTypeDef date_buffer;

        HAL_RTC_GetTime(&hrtc, &time_buffer, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &date_buffer, RTC_FORMAT_BIN);

        if (osMutexAcquire(rtc_handler.mutex, osWaitForever) == osOK)
        {
            rtc_handler.current_time = time_buffer;
            rtc_handler.current_date = date_buffer;

            if (osMutexRelease(rtc_handler.mutex) != osOK)
            {
                handle_error();
            }
        }
        else
        {
            handle_error();
        }

        osDelay(1000);
    }
}

static bool rtc_reset_time_and_date(void)
{
    RTC_TimeTypeDef default_time = {0};
    RTC_DateTypeDef default_date = {0};

    default_time.Hours = 0;
    default_time.Minutes = 0;
    default_time.Seconds = 0;

    default_date.WeekDay = RTC_WEEKDAY_MONDAY;
    default_date.Date = 1;
    default_date.Month = RTC_MONTH_JANUARY;
    default_date.Year = 0;

    bool time_ok = HAL_RTC_SetTime(&hrtc, &default_time, RTC_FORMAT_BIN) == HAL_OK;
    bool date_ok = HAL_RTC_SetDate(&hrtc, &default_date, RTC_FORMAT_BIN) == HAL_OK;

    return time_ok && date_ok;
}

static void handle_error(void)
{

}
