/**
 *******************************************************************************
 * @file 	stm32g4_systick.c
 * @author 	jjo
 * @date 	Mar 29, 2024
 * @brief 	Board Specific Package permettant l'ajout de callback pour le timer system
 *******************************************************************************
 */

#ifndef BSP_STM32G4_SYSTICK_H_
#define BSP_STM32G4_SYSTICK_H_

/* Includes ------------------------------------------------------------------*/
#include "stm32g4_utils.h"

/* Public define -------------------------------------------------------------*/
#define SYSTEM_CLOCK_MHZ 170

/* Public functions declarations ---------------------------------------------*/
void BSP_systick_init(void);

bool BSP_systick_add_callback_function(callback_fun_t func);

bool BSP_systick_remove_callback_function(callback_fun_t func);

uint32_t BSP_systick_get_time_us(void);

#endif /* BSP_STM32G4_SYSTICK_H_ */
