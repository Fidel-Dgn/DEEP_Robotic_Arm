/**
 *******************************************************************************
 * @file	stm32g4_motorDC.h
 * @author	vchav
 * @date	Jun 13, 2024
 * @brief	
 *******************************************************************************
 */
#ifndef BSP_MOTORDC_STM32G4_MOTORDC_H_
#define BSP_MOTORDC_STM32G4_MOTORDC_H_
#include "config.h"
#if USE_MOTOR_DC
#include "stm32g4_utils.h"


typedef enum
{
	MOTOR_ID_0,
	MOTOR_ID_1,
	MOTOR_ID_2,
	MOTOR_ID_3,
	MOTOR_NB,		//nombre max de moteurs !
	MOTOR_ID_NONE =-1
}motor_id_e;

void BSP_MOTOR_demo(void);

running_t BSP_MOTOR_demo_with_manual_drive (bool ask_for_finish, char touch_pressed);

motor_id_e BSP_MOTOR_add(GPIO_TypeDef * gpio_forward, uint16_t pin_forward, GPIO_TypeDef * gpio_reverse, uint16_t pin_reverse);

void BSP_MOTOR_set_duty(motor_id_e id, int16_t duty);

#endif
#endif /* BSP_MOTORDC_STM32G4_MOTORDC_H_ */
