
#include "config.h"
#include "stm32g4_uart.h"
#include "stm32g4_utils.h"
#include <servo.h>
#include "stm32g4_timer.h"


// N'oubliez pas les handles externes pour la macro HAL
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

// Votre implémentation de SERVO_set_angle comme vu précédemment...

// Importation des handles définis ailleurs dans le projet
#define PERIOD_TIMER 10 //ms

static uint16_t current_position;

void SERVO_process_test(timer_id_t timer_id,uint16_t TIM_CHANNEL_x){

	SERVO_set_position(timer_id,TIM_CHANNEL_x,0);
	HAL_Delay(5000);
	SERVO_set_position(timer_id,TIM_CHANNEL_x,100);
	HAL_Delay(5000);
	SERVO_set_position(timer_id,TIM_CHANNEL_x,50);
	HAL_Delay(5000);
	SERVO_set_position(timer_id,TIM_CHANNEL_x,100);


	 static uint16_t position = 50;
	 if( BSP_UART_button(UART2_ID) ){

		 //si appui bouton
		 position = (position > 99)?0:(position+5); //de 0 à 100%, par pas de 5%
		 SERVO_set_position(timer_id,TIM_CHANNEL_x,position);
	 }

}


void SERVO_init(void){

		//initialisation et lancement du timer1 à une période de 10 ms
		BSP_TIMER_run_us(TIMER3_ID, PERIOD_TIMER*2000, false); //20000us = 20ms*
		BSP_TIMER_run_us(TIMER1_ID, PERIOD_TIMER*2000, false); //20000us = 20ms*
		BSP_TIMER_run_us(TIMER2_ID, PERIOD_TIMER*2000, false); //20000us = 20ms*

				/*
				 * SERVO_process_test(TIMER3_ID, TIM_CHANNEL_1);PB4
				SERVO_process_test(TIMER3_ID, TIM_CHANNEL_3);PB0
				SERVO_process_test(TIMER1_ID, TIM_CHANNEL_3);PA10
				SERVO_process_test(TIMER2_ID, TIM_CHANNEL_1);PA0
				SERVO_process_test(TIMER2_ID, TIM_CHANNEL_2);PA1*/


		//activation du signal PWM sur les canaux
		while(1){
			/*BSP_TIMER_enable_PWM(TIMER3_ID, TIM_CHANNEL_1, 125, true, false);
			HAL_Delay(3000);
			BSP_TIMER_enable_PWM(TIMER3_ID, TIM_CHANNEL_1, 25, true, false);
			HAL_Delay(3000);
			BSP_TIMER_enable_PWM(TIMER3_ID, TIM_CHANNEL_3, 100, false, false); //fermeture
			HAL_Delay(3000);
			BSP_TIMER_enable_PWM(TIMER3_ID, TIM_CHANNEL_3, 25, false, false); // ouverture
			HAL_Delay(3000);
			BSP_TIMER_enable_PWM(TIMER3_ID, TIM_CHANNEL_3, 50, false, false); // bras
			HAL_Delay(3000);*/
			//BSP_TIMER_enable_PWM(TIMER1_ID, TIM_CHANNEL_3, 50, false, false);
			//HAL_Delay(1000);
			BSP_TIMER_enable_PWM(TIMER2_ID, TIM_CHANNEL_1, 25, false, true);
			HAL_Delay(1000);
			BSP_TIMER_enable_PWM(TIMER2_ID, TIM_CHANNEL_2, 25, false, false);
			HAL_Delay(1000);


		}

		BSP_TIMER_enable_PWM(TIMER3_ID, TIM_CHANNEL_3, 50, false, false);
		//BSP_TIMER_enable_PWM(TIMER3_ID, TIM_CHANNEL_4, 150, false, false);
		BSP_TIMER_enable_PWM(TIMER1_ID, TIM_CHANNEL_3, 50, false, false);
		BSP_TIMER_enable_PWM(TIMER2_ID, TIM_CHANNEL_1, 50, false, false);
		BSP_TIMER_enable_PWM(TIMER2_ID, TIM_CHANNEL_2, 50, false, false);

		//rapport cyclique reglé pour une position servo de 50%
		SERVO_set_position(TIMER3_ID, TIM_CHANNEL_1,100);
		SERVO_set_position(TIMER3_ID, TIM_CHANNEL_3,0);
		SERVO_set_position(TIMER2_ID, TIM_CHANNEL_1,50);
		SERVO_set_position(TIMER1_ID, TIM_CHANNEL_3,0);
		SERVO_set_position(TIMER2_ID, TIM_CHANNEL_2,0);
	 }




//position est exprimée de 0 à 100.
void SERVO_set_position(timer_id_t timer_id,uint16_t TIM_CHANNEL_x,uint16_t position){

		if(position > 100)
			position = 100; //écretage si l'utilisateur demande plus de 100%
		current_position = position;
		// Conversion de la position en rapport cyclique (‰)
		uint16_t duty_per_mille = position + 100;  // de 100 à 200 pour 0% à 100%

		// Appel de la fonction timer avec conversion interne en ticks
		BSP_TIMER_set_duty(timer_id, TIM_CHANNEL_x, duty_per_mille);

	}
/*void SERVO_set_angle(timer_id_t timer, uint32_t channel, uint16_t angle) {
    if (angle > 180) angle = 180;
    // Calcul du Pulse (0.5ms à 2.5ms pour 0-180°)
    // Dépend de ta configuration ARR et PSC du Timer
    uint32_t pulse = 500 + (angle * 2000 / 180);
    __HAL_TIM_SET_COMPARE(get_timer_handle(timer), channel, pulse);
}*/
void SERVO_set_angle(timer_id_t timer, uint32_t channel, uint16_t angle) {
    if (angle > 180) angle = 180;

    // Calcul du Pulse (0.5ms à 2.5ms soit environ 5% à 12.5% du rapport cyclique)
    // Attention : ces valeurs dépendent de votre ARR (Auto-Reload Register).
    // Si ARR = 20000, alors 0° = 1000 et 180° = 2500.
    uint32_t pulse = 20000 + (angle * 1000 / 180);

    if (timer == TIMER2_ID) {
        __HAL_TIM_SET_COMPARE(&htim2, channel, pulse);
    }
    else if (timer == TIMER3_ID) {
        __HAL_TIM_SET_COMPARE(&htim3, channel, pulse);
    }
    else if (timer == TIMER1_ID) {
        __HAL_TIM_SET_COMPARE(&htim1, channel, pulse);
    }
}

