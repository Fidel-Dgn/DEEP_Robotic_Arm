/**
 *******************************************************************************
 * @file	stm32g4_extit.c
 * @author	jjo
 * @date	Apr 28, 2024
 * @brief	Gestion des interruptions externes pour cible STM32G4
 * 			Adaptation du module stm32f1_extit.c de S.Poiraud
 *******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/
#include "stm32g4_extit.h"

#if USE_BSP_EXTIT
/* Private defines -----------------------------------------------------------*/


/* Private types -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
static callback_extit_t callbacks[16] = {0};
static uint16_t enables = 0;

/* Private constants ---------------------------------------------------------*/


/* Private functions declarations --------------------------------------------*/


/* Private functions definitions ---------------------------------------------*/
/**
 * @brief Cette fonction appel la fonction de callback associée à la broche passée en paramètre
 *
 * Cette fonction est appelée par les fonctions d'interruption EXTIx_IRQHandler
 * @param pin_number : numéro de la broche pour laquelle appeler la fonction de callback (entier compris entre 0 et 15)
 */
static void call_extit_user_callback(uint8_t pin_number)
{
	uint16_t gpio_pin;
	gpio_pin = (uint16_t)(1) << (uint16_t)(pin_number);
	if(__HAL_GPIO_EXTI_GET_IT(gpio_pin))
	{
		__HAL_GPIO_EXTI_CLEAR_IT(gpio_pin);
		if(enables & gpio_pin)
		{
			if(callbacks[pin_number])
				(*callbacks[pin_number])(pin_number);
		}
	}
}

/* Public functions definitions ----------------------------------------------*/
/**
 * @brief Cette fonction permet de déclarer une fonction de callback associée à un numéro de broche
 *
 * @param fun		: fonction de callback
 * @param pin_number: numéro de broche associée au callback
 * @param enable	: activer ou non les interruptions externes pour cette broche
 */
void BSP_EXTIT_set_callback(callback_extit_t fun, uint8_t pin_number, bool enable)
{
	callbacks[pin_number] = fun;
	if(enable)
		BSP_EXTIT_enable(pin_number);
}

/**
 * @brief Cette fonction autorise les interruptions externes correspondant au numéro de broche demandé
 *
 * @param pin_number : numéro de la broche sur laquelle activer les interruptions externes
 */
void BSP_EXTIT_enable(uint8_t pin_number)
{
	if(pin_number < 16)
		BIT_SET(enables, pin_number);
	switch(pin_number)
	{
		case 0:	NVIC_EnableIRQ(EXTI0_IRQn);	break;
		case 1:	NVIC_EnableIRQ(EXTI1_IRQn);	break;
		case 2:	NVIC_EnableIRQ(EXTI2_IRQn);	break;
		case 3:	NVIC_EnableIRQ(EXTI3_IRQn);	break;
		case 4:	NVIC_EnableIRQ(EXTI4_IRQn);	break;
		default:
			if(pin_number < 10)
				NVIC_EnableIRQ(EXTI9_5_IRQn);
			else if(pin_number < 16)
				NVIC_EnableIRQ(EXTI15_10_IRQn);
			break;
	}
}

/**
 * @brief Cette fonction désactive les interruptions externes correspondant au numéro de broche demandé
 *
 * À partir de la broche 5, les interruptions sont regroupées dans EXTI9_5_IRQn et EXTI15_10_IRQn,
 * L'interruption correpondante ne sera désactivée que si aucune autre source partageant la même it n'est active.
 * @param pin_number : numéro de la broche sur laquelle désactiver les interruptions externes
 */
void BSP_EXTIT_disable(uint8_t pin_number)
{
	if(pin_number < 16)
		BIT_CLR(enables, pin_number);

	switch(pin_number)
	{
		case 0:	NVIC_DisableIRQ(EXTI0_IRQn);	break;
		case 1:	NVIC_DisableIRQ(EXTI1_IRQn);	break;
		case 2:	NVIC_DisableIRQ(EXTI2_IRQn);	break;
		case 3:	NVIC_DisableIRQ(EXTI3_IRQn);	break;
		case 4:	NVIC_DisableIRQ(EXTI4_IRQn);	break;
		default:
			if(pin_number < 10)
			{
				if(((enables >> 5)&0b11111) == 0)
					NVIC_DisableIRQ(EXTI9_5_IRQn);
			}
			else if(pin_number < 16)
			{
				if(enables >> 10 == 0)
					NVIC_DisableIRQ(EXTI15_10_IRQn);
			}
			break;
	}
}

/**
 * @brief Cette fonction permet de transcrire la position de la broche au format
 * GPIO_PIN_x en un numéro de broche
 *
 * @param GPIO_PIN_x : La position de la broche au format GPIO_PIN_x
 * @return : le numéro de la broche (utilisable dans les fonctions BSP_EXTIT)
 */
uint8_t BSP_EXTIT_gpiopin_to_pin_number(uint16_t GPIO_PIN_x)
{
	uint8_t ret = -1;
	switch(GPIO_PIN_x)
	{
		case GPIO_PIN_0:	ret = 0;	break;
		case GPIO_PIN_1:	ret = 1;	break;
		case GPIO_PIN_2:	ret = 2;	break;
		case GPIO_PIN_3:	ret = 3;	break;
		case GPIO_PIN_4:	ret = 4;	break;
		case GPIO_PIN_5:	ret = 5;	break;
		case GPIO_PIN_6:	ret = 6;	break;
		case GPIO_PIN_7:	ret = 7;	break;
		case GPIO_PIN_8:	ret = 8;	break;
		case GPIO_PIN_9:	ret = 9;	break;
		case GPIO_PIN_10:	ret = 10;	break;
		case GPIO_PIN_11:	ret = 11;	break;
		case GPIO_PIN_12:	ret = 12;	break;
		case GPIO_PIN_13:	ret = 13;	break;
		case GPIO_PIN_14:	ret = 14;	break;
		case GPIO_PIN_15:	ret = 15;	break;
		default:
			break;
	}
	return ret;
}

/**
 * @brief Cette fontion acquitte l'interruption externe correspondant au numéro de broche passé en paramètre
 *
 * @param pin_number : numéro de la broche dont on veut acquitter l'interruption
 */
void BSP_EXTIT_ack_it(uint8_t pin_number)
{
	uint16_t gpio_pin;
	gpio_pin = (uint16_t)(1) << (uint16_t)(pin_number);
	if(__HAL_GPIO_EXTI_GET_IT(gpio_pin))
		__HAL_GPIO_EXTI_CLEAR_IT(gpio_pin);
}
/**
 * @brief 	Routine d'interruption appelée AUTOMATIQUEMENT lorsque d'une interruption externe sur une broche PX0.
 * @pre		Cette fonction NE DOIT PAS être appelée directement par l'utilisateur...
 * @post	Acquittement du flag d'interruption, et appel de la fonction de callback rensignée par l'utilisateur (si elle existe)
 * @note	Nous n'avons PAS le choix du nom de cette fonction, c'est comme ça qu'elle est nommée dans le fichier startup.s !
 */
void EXTI0_IRQHandler(void)
{
	call_extit_user_callback(0);
}

void EXTI1_IRQHandler(void)
{
	call_extit_user_callback(1);
}

void EXTI2_IRQHandler(void)
{
	call_extit_user_callback(2);
}

void EXTI3_IRQHandler(void)
{
	call_extit_user_callback(3);
}

void EXTI4_IRQHandler(void)
{
	call_extit_user_callback(4);
}


void EXTI9_5_IRQHandler(void)
{
	call_extit_user_callback(5);
	call_extit_user_callback(6);
	call_extit_user_callback(7);
	call_extit_user_callback(8);
	call_extit_user_callback(9);
}

void EXTI15_10_IRQHandler(void)
{
	call_extit_user_callback(10);
	call_extit_user_callback(11);
	call_extit_user_callback(12);
	call_extit_user_callback(13);
	call_extit_user_callback(14);
	call_extit_user_callback(15);
}

#endif /* USE_BSP_EXTIT */
