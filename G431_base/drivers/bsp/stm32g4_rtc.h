/**
 *******************************************************************************
 * @file	stm32g4_rtc.h
 * @author	vchav
 * @date	May 31, 2024
 * @brief	Module pour utiliser la RTC (Real Time Clock)
 *******************************************************************************
 */
#ifndef BSP_STM32G4_RTC_H_
#define BSP_STM32G4_RTC_H_

#include "config.h"
#if USE_RTC
#include "stm32g4_utils.h"
#include "stm32g4xx_hal_rtc.h"

typedef enum {
    JANUARY 	= RTC_MONTH_JANUARY,
    FEBRUARY 	= RTC_MONTH_FEBRUARY,
    MARCH 		= RTC_MONTH_MARCH,
    APRIL 		= RTC_MONTH_APRIL,
    MAY 		= RTC_MONTH_MAY,
    JUNE 		= RTC_MONTH_JUNE,
    JULY 		= RTC_MONTH_JULY,
    AUGUST 		= RTC_MONTH_AUGUST,
    SEPTEMBER 	= RTC_MONTH_SEPTEMBER,
    OCTOBER 	= RTC_MONTH_OCTOBER,
    NOVEMBER 	= RTC_MONTH_NOVEMBER,
    DECEMBER 	= RTC_MONTH_DECEMBER
} month_e;

typedef enum {
    MONDAY 		= RTC_WEEKDAY_MONDAY,
    TUESDAY 	= RTC_WEEKDAY_TUESDAY,
    WEDNESDAY 	= RTC_WEEKDAY_WEDNESDAY,
    THURSDAY 	= RTC_WEEKDAY_THURSDAY,
    FRIDAY 		= RTC_WEEKDAY_FRIDAY,
    SATURDAY 	= RTC_WEEKDAY_SATURDAY,
    SUNDAY 		= RTC_WEEKDAY_SUNDAY
} weekday_e;

typedef enum{
	WEEKDAY = 0,
	DATE
}alarm_mode_e;

typedef enum{
	ALARM_A,
	ALARM_B
}alarm_e;

void BSP_RTC_init(void);

void BSP_RTC_set_date(RTC_DateTypeDef *sDate);

void BSP_RTC_set_time(RTC_TimeTypeDef *sTime);

void BSP_RTC_get_time_and_date(RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate);

void BSP_RTC_get_date(RTC_DateTypeDef *sDate);

void BSP_RTC_set_alarm(alarm_e alarm, uint8_t hour, uint8_t min, uint8_t sec, alarm_mode_e mode,  uint8_t weekdaydate, bool enable_interrupt, bool * flag);

void BSP_RTC_reset_alarm(alarm_e alarm);

void BSP_RTC_get_alarm(RTC_AlarmTypeDef *sAlarm, uint32_t Alarm);

void BSP_RTC_set_time_acceleration(uint32_t time_acceleration);

running_t DEMO_RTC_process_main(bool ask_for_finish);


#endif
#endif /* BSP_STM32G4_RTC_H_ */
