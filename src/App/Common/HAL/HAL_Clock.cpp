#include "HAL.h"
#include "ChappieCore/ChappieCore.h"

extern ChappieCore Chappie;

void HAL::Clock_Init()
{
    // ...
}

void HAL::Clock_GetInfo(Clock_Info_t *info)
{
    // Use system time instead of RTC to avoid I2C errors
    // when RTC hardware is not connected
    struct tm timeinfo;
    time_t now = time(nullptr);
    localtime_r(&now, &timeinfo);
    
    info->year = timeinfo.tm_year + 1900;
    info->month = timeinfo.tm_mon + 1;
    info->day = timeinfo.tm_mday;
    info->week = timeinfo.tm_wday;
    info->hour = timeinfo.tm_hour;
    info->minute = timeinfo.tm_min;
    info->second = timeinfo.tm_sec;
    info->millisecond = 0;
    
    // Alternative: If RTC hardware is available, use this instead:
    /*
    // Get RTC time
    I2C_BM8563_TimeTypeDef timeStruct;
    // Get RTC Date
    I2C_BM8563_DateTypeDef dateStruct;

    Chappie.Rtc.getTime(&timeStruct);
    Chappie.Rtc.getDate(&dateStruct);

    info->year = dateStruct.year;
    info->month = dateStruct.month;
    info->day = dateStruct.date;
    info->week = dateStruct.weekDay;
    info->hour = timeStruct.hours;
    info->minute = timeStruct.minutes;
    info->second = timeStruct.seconds;
    info->millisecond = 0;
    */
}

void HAL::Clock_SetInfo(const Clock_Info_t *info)
{
    // Set system time instead of RTC to avoid I2C errors
    // when RTC hardware is not connected
    struct tm timeinfo = {0};
    timeinfo.tm_year = info->year - 1900;
    timeinfo.tm_mon = info->month - 1;
    timeinfo.tm_mday = info->day;
    timeinfo.tm_hour = info->hour;
    timeinfo.tm_min = info->minute;
    timeinfo.tm_sec = info->second;
    
    time_t now = mktime(&timeinfo);
    struct timeval tv = { now, 0 };
    settimeofday(&tv, NULL);
    
    // Alternative: If RTC hardware is available, use this instead:
    /*
    // Set RTC time
    I2C_BM8563_TimeTypeDef timeStruct;
    timeStruct.hours = info->hour;
    timeStruct.minutes = info->minute;
    timeStruct.seconds = info->second;
    Chappie.Rtc.setTime(&timeStruct);

    // Set RTC Date
    I2C_BM8563_DateTypeDef dateStruct;
    dateStruct.month = info->month;
    dateStruct.date = info->day;
    dateStruct.year = info->year;
    Chappie.Rtc.setDate(&dateStruct);
    */
}

const char *HAL::Clock_GetWeekString(uint8_t week)
{
    const char *week_str[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    return week < 7 ? week_str[week] : "";
}
