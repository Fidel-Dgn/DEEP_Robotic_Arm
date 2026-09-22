/**
 *******************************************************************************
 * @file	stm32g4_matrix_keyboard.h
 * @author	vchav
 * @date	Jun 10, 2024
 * @brief	Module pour utiliser un clavier matriciel (adaptation du module de Samuel Poiraud sur f103)
 *******************************************************************************
 */

#ifndef BSP_MATRIXKEYBOARD_STM32G4_MATRIX_KEYBOARD_H_
#define BSP_MATRIXKEYBOARD_STM32G4_MATRIX_KEYBOARD_H_

#include "config.h"
#if USE_MATRIX_KEYBOARD
#include <stdbool.h>

// valeurs possibles :
	//			GPIO du STM32G4					|	GPIOs de l'expander MCP23S17
	// ports : 	GPIOA, GPIOB, 					|	MCP23S17_PORT_A, MCP23S17_PORT_B
	// pins : 	GPIO_PIN_x (x from 0 to 15) 	|	MCP23S17_PIN_x (x from 0 to 7)
#define PORT_INPUT_0 	GPIOA
#define PORT_INPUT_1 	GPIOA
#define PORT_INPUT_2 	GPIOA
#define PORT_INPUT_3 	GPIOB
#define	PIN_INPUT_0		GPIO_PIN_9
#define	PIN_INPUT_1		GPIO_PIN_10
#define	PIN_INPUT_2		GPIO_PIN_12
#define	PIN_INPUT_3		GPIO_PIN_0

#define PORT_OUTPUT_0 	GPIOB
#define PORT_OUTPUT_1 	GPIOA
#define PORT_OUTPUT_2 	GPIOA
#define PORT_OUTPUT_3 	GPIOA
#define	PIN_OUTPUT_0	GPIO_PIN_7
#define	PIN_OUTPUT_1	GPIO_PIN_15
#define	PIN_OUTPUT_2	GPIO_PIN_1
#define	PIN_OUTPUT_3	GPIO_PIN_0

#define MATRIX_KEYBOARD_HAL_CLOCK_ENABLE()	__HAL_RCC_GPIOA_CLK_ENABLE()

#define CONFIG_PULL_UP
//#define CONFIG_PULL_DOWN


#define NO_KEY 		0xFF
#define MANY_KEYS 	0xFE


void BSP_MATRIX_KEYBOARD_init(const char * new_keyboard_keys);
void BSP_MATRIX_KEYBOARD_demo_process_main(void);
void BSP_MATRIX_KEYBOARD_demo_process_1ms(void);
bool BSP_MATRIX_KEYBOARD_is_pressed(void);
char BSP_MATRIX_KEYBOARD_get_key(void);
void BSP_MATRIX_KEYBOARD_press_and_release_events(char * press_event, char * release_event, uint32_t * all_touchs_pressed);

#endif
#endif /* BSP_MATRIXKEYBOARD_STM32G4_MATRIX_KEYBOARD_H_ */
