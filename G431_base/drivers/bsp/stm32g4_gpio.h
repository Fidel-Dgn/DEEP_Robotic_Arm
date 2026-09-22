/**
 *******************************************************************************
 * @file 	stm32g4_gpio.h
 * @author 	jjo
 * @date 	Mar 29, 2024
 * @brief	Mdule GPIO pour cible stm32g4
 *******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BSP_STM32G4_GPIO_H__
#define BSP_STM32G4_GPIO_H__

/* Includes ------------------------------------------------------------------*/
#include "config.h"

/* Defines -------------------------------------------------------------------*/
#define GPIO_NO_AF	0	// For configuration with no alternate function (pure GPIO)

/* Public functions declarations ---------------------------------------------*/

void BSP_GPIO_enable(void);

void BSP_GPIO_pin_config(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin, uint32_t GPIO_Mode, uint32_t GPIO_Pull, uint32_t GPIO_Speed, uint32_t GPIO_Alternate);

#endif /* BSP_STM32G4_GPIO_H__ */

