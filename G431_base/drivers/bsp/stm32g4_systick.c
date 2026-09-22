/**
 *******************************************************************************
 * @file 	stm32g4_systick.c
 * @author 	jjo
 * @date 	Mar 29, 2024
 * @brief 	Board Specific Package permettant l'ajout de callback pour le timer system
 *			Adaptation du module systick.c de S.Poiraud
 *******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/
#include "stm32g4_systick.h"
#include "stm32g4xx_hal.h"

/* Private defines -----------------------------------------------------------*/
#define MAX_CALLBACK_FUNCTION_NB	16

/* Private variables ---------------------------------------------------------*/
/* Tableau de pointeurs sur fonctions qui doivent être appelées périodiquement (1ms) par l'IT systick. */
static callback_fun_t callback_functions[MAX_CALLBACK_FUNCTION_NB];
static bool initialized = false;


/* Public functions definitions ----------------------------------------------*/

/**
 * @brief Initialization function for high level Systick service
 *
 * @post The systick interruption priority is set and the callback function table is initialized
 */
void BSP_systick_init(void)
{
	uint8_t i;
	for(i = 0; i<MAX_CALLBACK_FUNCTION_NB; i++)
		callback_functions[i] = NULL;
	HAL_NVIC_SetPriority(SysTick_IRQn , 0,  0);
	initialized = true;
}


/**
 * @brief Add a callback function to the table
 *
 * @param func the function to be called as callback
 * @return true if the function was added, false otherwise
 */
bool BSP_systick_add_callback_function(callback_fun_t func)
{
	uint8_t i;
	if(!initialized)
		BSP_systick_init();

	for(i = 0; i<MAX_CALLBACK_FUNCTION_NB; i++)
	{
		if(!callback_functions[i])	//On a trouvé une place libre ?
		{
			callback_functions[i] = func;
			return true;
		}
	}
	return false;	//Pas de place libre !

}

/**
 * @brief Remove the callback function from the table
 *
 * @pre	The callback function exists
 * @param func the callback function to remove
 * @return true if the callback function was removed, false otherwise
 */
bool BSP_systick_remove_callback_function(callback_fun_t func)
{
	uint8_t i;
	if(!initialized)
		BSP_systick_init();
	for(i = 0; i<MAX_CALLBACK_FUNCTION_NB; i++)
	{
		if(callback_functions[i] == func)
		{
			callback_functions[i] = NULL;
			return true;
		}
	}
	return false;
}

uint32_t BSP_systick_get_time_us(void)
{
	uint32_t t_us;
	static uint32_t previous_t_us = 0;
	__disable_irq();
	t_us = HAL_GetTick() * 1000 + 1000 - SysTick->VAL / SYSTEM_CLOCK_MHZ;
	__enable_irq();


	if(previous_t_us >= t_us)
		t_us += 1000;
	previous_t_us = t_us ;

	return t_us;
}


/**
 * @brief Interrupt function called every 1ms
 *
 */
void SysTick_Handler(void)
{
	/* Minimum interruption job for SysTick */
	HAL_IncTick();
	/* Use of HAL_SYSTICK_IRQHandler() as been discouraged by ST and is not generated anymore by CubeMX */

	if(!initialized)
		BSP_systick_init();

	/* Management of the callback functions */
	uint8_t i;
	for(i = 0; i<MAX_CALLBACK_FUNCTION_NB; i++)
	{
		if(callback_functions[i])
			(*callback_functions[i])();		/* Function calls. */
	}
}
