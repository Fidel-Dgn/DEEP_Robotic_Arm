/*
 * stm32f1_stepper_motor.h
 *
 *  Created on: 29 nov. 2022
 *      Author: julia
 */

#ifndef BSP_STM32G4_STEPPER_MOTOR_H_
#define BSP_STM32G4_STEPPER_MOTOR_H_

#include "stm32g4_utils.h"

// définir ici le nombre de moteurs, en adaptant l'enumeration motor_id_e.... et en laissant MOTOR_NB à la fin de la liste des moteurs.
typedef enum
{
	STEPPER_MOTOR_0,
	STEPPER_MOTOR_1,	//commenter cette ligne si vous utilisez un seul moteur.
	STEPPER_MOTOR_NB
}motor_id_e;


void STEPPER_MOTOR_init(void);
void STEPPER_MOTOR_demo(void);
void STEPPER_MOTOR_set_callback_at_each_pulse(callback_fun_t cb);
void STEPPER_MOTOR_enable(motor_id_e id, bool enable);
void STEPPER_MOTOR_set_goal(motor_id_e id, int32_t newgoal);
void STEPPER_MOTOR_set_position(motor_id_e id, int32_t newposition);
int32_t STEPPER_MOTOR_get_position(motor_id_e id);
bool STEPPER_MOTOR_is_arrived (motor_id_e id);
int32_t STEPPER_MOTOR_get_goal (motor_id_e id);



#endif /* BSP_STM32G4_STEPPER_MOTOR_H_ */
