/**
 *******************************************************************************
 * @file	stm32g4_rtc.c
 * @author	vchav
 * @date	May 31, 2024
 * @brief	Module pour utiliser la RTC (Real Time Clock)
 *******************************************************************************
 */
#include "config.h"
#if USE_RTC
#include "stm32g4xx_hal_rtc.h"
#include "stm32g4_rtc.h"
#include "stm32g4_sys.h"
#include "stm32g4_uart.h"
#include <stdio.h>

static RTC_HandleTypeDef rtc_handle;
static bool initialized = false;
static volatile bool * alarm_interrupt_flag = NULL;

#define DEFAULT_ASYNCH_PREDIV	127
#define DEFAULT_SYNCH_PREDIV	255
#define PREDIV_SPLIT_SHIFT		4		// Décalage de 4 (division par 16) pour répartir la valeur d'accélération sur les deux prédiviseurs

static uint8_t RTC_WeekDayNum(uint8_t nYear, uint8_t nMonth, uint8_t nDay);

/**
 * @brief 	Cette fonction de démo permet de prendre en main facilement les quelques fonctions du module
 * 			et de vérifier que la RTC et les alarmes fonctionnent
 * @param ask_for_finish: tout est dans le nom
 * @return l'état de où en est la démo (IN_PROGRESS, END_OK)
 */
running_t DEMO_RTC_process_main(bool ask_for_finish)
{
	typedef enum
	{
		INIT = 0,
		RUN
	}state_e;
	static state_e state = INIT;
	running_t ret;
	ret = IN_PROGRESS;
	static bool flag_alarm;
	const char * weekday_str[7] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};

	switch(state)
	{
		case INIT: {
			state = RUN;
			BSP_RTC_init();
			RTC_TimeTypeDef time = {9, 42, 50};
			RTC_DateTypeDef date = {SUNDAY, MAY, 4, 24};
			BSP_RTC_set_time(&time);
			BSP_RTC_set_date(&date);
			BSP_RTC_set_alarm(ALARM_A, 5, 1, 5, WEEKDAY, MONDAY, true, &flag_alarm);

			printf("This demo will print the time every second.\n");
			printf("Commands :\n");
			printf("h : help\n");
			printf("r : reset time & date to default\n");

			break;}
		case RUN:{
			static uint8_t previous_printed_time = 0;
			RTC_TimeTypeDef time;
			RTC_DateTypeDef date;
			BSP_RTC_get_time_and_date(&time, &date);
			if(time.Seconds != previous_printed_time)
			{
				printf("%s %2d/%2d/%2d - %2d:%2d:%2d\n", weekday_str[date.WeekDay], date.Date, date.Month, date.Year, time.Hours, time.Minutes, time.Seconds);
				previous_printed_time = time.Seconds;
			}

			uint8_t c;
			if(BSP_UART_data_ready(UART2_ID))
			{
				c = BSP_UART_getc(UART2_ID);
				switch(c)
				{
					case 'r':
						printf("reset time & date\n");
						RTC_TimeTypeDef time = {00, 00, 00};
						RTC_DateTypeDef date = {RTC_WEEKDAY_MONDAY, RTC_MONTH_APRIL, 1, 20};
						BSP_RTC_set_time(&time);
						BSP_RTC_set_date(&date);
						break;
					case 'h':
						printf("This demo will print the time every second.\n");
						printf("Commands :\n");
						printf("h : help\n");
						printf("r : reset time & date to default\n");
						break;
				}
			}
			if(flag_alarm)
			{
				printf("alarm occured\n");
				flag_alarm = false;
			}
			if(ask_for_finish)
			{
				state = INIT;
				ret = END_OK;
			}
			break;}
		default:
			break;
	}
	return ret;
}

/**
 * @brief 	Fonction pour initialiser la RTC basée sur l'horloge interne de la carte.
 * 			Cette horloge n'est pas la plus précise mais l'est suffisamment pour la plupart des projets faits à l'ESEO.
 */
void BSP_RTC_init(void){

	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
	  Error_Handler();
	}
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();

	rtc_handle.Instance = RTC;
	rtc_handle.Init.HourFormat = RTC_HOURFORMAT_24;
	rtc_handle.Init.AsynchPrediv = 127;
	rtc_handle.Init.SynchPrediv = 255;
	rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE;
	rtc_handle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
	rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	rtc_handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	rtc_handle.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;

	// Set year back to 00 to allow modification of init registers
	RTC_DateTypeDef null_date = {0};
	HAL_RTC_SetDate(&rtc_handle, &null_date, RTC_FORMAT_BIN);

	if (HAL_RTC_Init(&rtc_handle) != HAL_OK){
		Error_Handler();
	}
	initialized = true;
}

/**
 * @brief Fonction pour paramétrer la date
 * @param *sTime pointeur sur une structure de date RTC_DateTypedef
 */
void BSP_RTC_set_date(RTC_DateTypeDef *sDate)
{
	if(initialized)
	{
		// Calcul automatique du jour de la semaine
		sDate->WeekDay = RTC_WeekDayNum(sDate->Year, sDate->Month, sDate->Date);
		HAL_RTC_SetDate(&rtc_handle, sDate, RTC_FORMAT_BIN);
	}
}

/**
 * @brief Fonction pour paramétrer l'heure
 * @param *sTime pointeur sur une structure de temps RTC_TimeTypedef
 */
void BSP_RTC_set_time(RTC_TimeTypeDef *sTime)
{
	if(initialized)
		HAL_RTC_SetTime(&rtc_handle, sTime, RTC_FORMAT_BIN);
}


void BSP_RTC_get_time_and_date(RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate)
{
	if(initialized){
		HAL_RTC_GetTime(&rtc_handle, sTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&rtc_handle, sDate, RTC_FORMAT_BIN);
	}

}

/* BSP_RTC_get_time volontairement omise, la lecture de l'heure seule, sans la date, peut provoquer
 * le déclenchement d'un mécanisme de protection anticorruption qui provoque le blocage de la RTC
 * (http://www.efton.sk/STM32/gotcha/g6.html)
 */

void BSP_RTC_get_date(RTC_DateTypeDef *sDate)
{
	if(initialized)
		HAL_RTC_GetDate(&rtc_handle, sDate, RTC_FORMAT_BIN);
}

/**
 * @brief Fonction pour activer une alarme
 * @param alarm: 2 alarmes au choix ALARM_A et ALARM_B
 * @param hour: l'heure à laquelle l'alarme "sonnera"
 * @param min: la minute à laquelle l'alarme "sonnera"
 * @param sec: la seconde à laquelle l'alarme "sonnera"
 * @param mode: soit WEEKDAY --> définir l'alarme sur un jour précis de la semaine;
 * 				soit DATE --> définir l'alarme sur une date précise (ex:  le 14 mai (mon anniversaire:))
 * @param weekdaydate: 	si mode = WEEKDAY: on peut mettre une valeur de weekday_e
 * 						si mode = DATE --> on peut mettre une date entre 1 et 31
 * @param enable_interrupt: true si on veut déclencher une interruption lorsque l'alarme se déclenche
 * @param flag: pointeur vers le flag qui seras levé par l'interruption
 */
void BSP_RTC_set_alarm(alarm_e alarm, uint8_t hour, uint8_t min, uint8_t sec, alarm_mode_e mode,  uint8_t weekdaydate, bool enable_interrupt, bool * flag){
	RTC_AlarmTypeDef sAlarm = {0};
	if(hour > 23 ||  min > 59  || sec > 59 || !initialized)
		return;
	if(mode && (weekdaydate < 1 || weekdaydate > 31))
		return;
	if(!mode && (weekdaydate < 1 || weekdaydate > 7))
		return;
	sAlarm.AlarmTime.Hours = hour;
	sAlarm.AlarmTime.Minutes = min;
	sAlarm.AlarmTime.Seconds = sec;
	sAlarm.AlarmTime.SubSeconds = 0;
	sAlarm.AlarmMask = RTC_ALARMMASK_NONE; // On ne masque aucun byte de temps (jours+mois+heure) pour qu'ils soient tous utilisés dans la comparaison de l'alarme
	sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL; // On masque tous les bytes de sous-secondes pour qu'ils ne soient pas utilisés dans la comparaison de l'alarme.
	sAlarm.AlarmDateWeekDaySel = (!mode ? RTC_ALARMDATEWEEKDAYSEL_WEEKDAY : RTC_ALARMDATEWEEKDAYSEL_DATE);
	sAlarm.AlarmDateWeekDay = weekdaydate;
	sAlarm.Alarm = (alarm ? RTC_ALARM_B : RTC_ALARM_A);

	if(enable_interrupt){
		HAL_RTC_SetAlarm_IT(&rtc_handle, &sAlarm, RTC_FORMAT_BIN);
        HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0); // Configure la priorité de l'interruption
		HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
		if(flag){
			alarm_interrupt_flag = flag;	//on sauvegarde l'adresse du flag pour qu'il soit levé lors de l'interruption provoquée par l'alarme.
		}
	}else{
		HAL_RTC_SetAlarm(&rtc_handle, &sAlarm, RTC_FORMAT_BIN);
	}
}

void BSP_RTC_reset_alarm(alarm_e alarm){
	HAL_RTC_DeactivateAlarm(&rtc_handle, (alarm?RTC_ALARM_B:RTC_ALARM_A));
}

void RTC_IRQHandler(void){
	HAL_RTC_AlarmIRQHandler(&rtc_handle);
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc){
	if(alarm_interrupt_flag)
		*alarm_interrupt_flag = true;
}

void RTC_Alarm_IRQHandler(void) {
    HAL_RTC_AlarmIRQHandler(&rtc_handle);
}

void BSP_RTC_get_alarm(RTC_AlarmTypeDef *sAlarm, uint32_t Alarm){
	if(initialized)
		HAL_RTC_GetAlarm(&rtc_handle, sAlarm, Alarm, RTC_FORMAT_BIN);
}

/**
 * @brief 	Cette fonction permet de modifier la vitesse de la RTC (à des fins de tests notamment)
 * @param 	time_acceleration: 1 pour une vitesse normale. 60 pour un écoulement d'une minute par seconde. 5*60 pour 5mn/seconde...
 * @pre		valeur maximale: 32768 secondes par seconde.
 */
void BSP_RTC_set_time_acceleration(uint32_t time_acceleration)
{
	if(initialized)
	{
		// Sauvegarde de la date et remise à 00 du champ année pour permettre la modification des prédiviseurs
		RTC_DateTypeDef backup_date = {0};
		RTC_DateTypeDef null_date = {0};
		BSP_RTC_get_date(&backup_date);
		BSP_RTC_set_date(&null_date);

		if(time_acceleration >= 1<<PREDIV_SPLIT_SHIFT)
		{
			rtc_handle.Init.AsynchPrediv = DEFAULT_ASYNCH_PREDIV>>PREDIV_SPLIT_SHIFT;
			rtc_handle.Init.SynchPrediv = DEFAULT_SYNCH_PREDIV/(time_acceleration>>PREDIV_SPLIT_SHIFT);
		}
		else
		{
			rtc_handle.Init.SynchPrediv = DEFAULT_SYNCH_PREDIV/time_acceleration;
		}
		HAL_RTC_Init(&rtc_handle);	// Update RTC configuration

		// Remise en place de la date sauvegardée
		BSP_RTC_set_date(&backup_date);
	}
}

static uint8_t RTC_WeekDayNum(uint8_t nYear, uint8_t nMonth, uint8_t nDay)
{
  uint32_t year = 0U, weekday = 0U;

  year = 2000U + nYear;

  if(nMonth < 3U)
  {
    /*D = { [(23 x month)/9] + day + 4 + year + [(year-1)/4] - [(year-1)/100] + [(year-1)/400] } mod 7*/
    weekday = (((23U * nMonth)/9U) + nDay + 4U + year + ((year-1U)/4U) - ((year-1U)/100U) + ((year-1U)/400U)) % 7U;
  }
  else
  {
    /*D = { [(23 x month)/9] + day + 4 + year + [year/4] - [year/100] + [year/400] - 2 } mod 7*/
    weekday = (((23U * nMonth)/9U) + nDay + 4U + year + (year/4U) - (year/100U) + (year/400U) - 2U ) % 7U;
  }

  return (uint8_t)weekday;
}


#endif



