/**
 *******************************************************************************
 * @file	stm32g4_hcsr04.h
 * @author	Nirgal			&& Luc Hérault
 * @date	22 mai 2019		&& Juin 2024 --> portage sur g431
 * @brief	Module pour utiliser le HC-SR04
 *******************************************************************************
 */

#ifndef HCSR04_H_
#define HCSR04_H_
#include "config.h"
#if USE_HCSR04

typedef enum
{
	HCSR04_STATE_INEXISTANT = 0,
	HCSR04_STATE_INITIALIZED,
	HCSR04_STATE_TRIG,
	HCSR04_STATE_WAIT_ECHO_RISING,
	HCSR04_STATE_WAIT_ECHO_FALLING,
	HCSR04_STATE_ECHO_RECEIVED,
	HCSR04_STATE_TIMEOUT,
	HCSR04_STATE_ERROR,
	HCSR04_STATE_IDLE
}hcsr04_state_t;

#define PERIOD_MEASURE			100

#define HSCR04_TIMEOUT			150		//ms

#define US_SPEED_IN_AIR			344	//mm/ms (à environ 20°)

typedef struct
{
	GPIO_TypeDef * trig_gpio;
	uint16_t trig_pin;
	GPIO_TypeDef * echo_gpio;
	uint16_t echo_pin;
	hcsr04_state_t state;
	uint32_t ttrig;		//ms
	uint32_t tfalling;	//us
	uint32_t trising;	//us
	uint16_t distance;
}hcsr04_t;


HAL_StatusTypeDef BSP_HCSR04_add(uint8_t * id, GPIO_TypeDef * TRIG_GPIO, uint16_t TRIG_PIN, GPIO_TypeDef * ECHO_GPIO, uint16_t ECHO_PIN);
HAL_StatusTypeDef BSP_HCSR04_get_value(uint8_t id, uint16_t * distance);
void BSP_HCSR04_run_measure(uint8_t id);
void BSP_HCSR04_process_main(void);

#endif
#endif /* HCSR04_H_ */
