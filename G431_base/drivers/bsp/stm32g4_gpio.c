/**
 *******************************************************************************
 * @file 	stm32g4_gpio.c
 * @author 	jjo
 * @date 	Mar 29, 2024
 * @brief	Module GPIO pour cible stm32g4
 * 			Adaptation du module stm32f1_gpio.c de F.CHARRUAUD
 *******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32g4_gpio.h"
#include "stdbool.h"

/*
 * Ce module logiciel a pour but de simplifier l'utilisation des fonctions HAL_GPIO_...
 *
 * Attention à penser à appeler au moins une fois, lors de l'initialisation, la fonction :
 * 		BSP_GPIO_Enable();
 *
 * Initialiser une broche en entrée (exemple) :
 * 		BSP_GPIO_PinCfg(GPIOA,GPIO_PIN_0,GPIO_MODE_INPUT,GPIO_NO_PULL,GPIO_SPEED_HIGH,GPIO_NO_AF);
 * 			variante tirage pour appliquer des tirages haut ou bas : GPIO_PULLUP, GPIO_PULLDOWN
 *
 * Initialiser une broche en sortie (exemple) :
 * 		BSP_GPIO_PinCfg(GPIOA,GPIO_PIN_0,GPIO_MODE_OUTPUT_PP,GPIO_PULLUP,GPIO_SPEED_HIGH,GPIO_NO_AF); *
 *
 * Pour manipuler les broches ainsi configurées :
 * 		b = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);	-> renvoie 0 ou 1... selon l'état de la broche en entrée
 * 		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);		-> écrit 0 ou 1 sur la broche en sortie
 * 		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_0);					-> inverse l'état de la broche en sortie (0 <-> 1)
 *
 */


static bool initialized = false;
/**
 * @brief Activation des horloges des peripheriques GPIOx
 *
 */
void BSP_GPIO_enable(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_PWREx_DisableUCPDDeadBattery();	//désactive les pull-down sur PB4 et PB6 lorsque PA9 et PA10 sont à 1.
  initialized = true;
}

/**
 * @brief Fonction générale permettant de configurer une broche de GPIO
 *
 * @param GPIOx : peut-être GPIOA-G
 * @param GPIO_Pin : GPIO_PIN_X avec X correspondant à la (ou les) broche souhaitées
 * @param GPIO_Mode : GPIO_MODE_INPUT, GPIO_MODE_OUTPUT_PP, GPIO_MODE_OUTPUT_OD, GPIO_MODE_AF_PP, GPIO_MODE_AF_OD ou GPIO_MODE_ANALOG
 * @param GPIO_Pull : GPIO_NOPULL, GPIO_PULLUP, GPIO_PULLDOWN
 * @param GPIO_Speed : GPIO_SPEED_LOW (2MHz), GPIO_SPEED_MEDIUM (25MHz), GPIO_SPEED_HIGH (100MHz)
 * @param GPIO_Alternate : GPIO_AF0 à GPIO_AF15 ou GPIO_NO_AF pour une broche GPIO pure sans fonction alternative
 */
void BSP_GPIO_pin_config(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin, uint32_t GPIO_Mode, uint32_t GPIO_Pull, uint32_t GPIO_Speed, uint32_t GPIO_Alternate)
{
	GPIO_InitTypeDef GPIO_InitStructure = { 0 };//Structure contenant les arguments de la fonction GPIO_Init

	if(!initialized)
		BSP_GPIO_enable();

	GPIO_InitStructure.Pin = GPIO_Pin;
	GPIO_InitStructure.Mode = GPIO_Mode;
	GPIO_InitStructure.Pull = GPIO_Pull;
	GPIO_InitStructure.Speed = GPIO_Speed;
	GPIO_InitStructure.Alternate = GPIO_Alternate;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStructure);
}


