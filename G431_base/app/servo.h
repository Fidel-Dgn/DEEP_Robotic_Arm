
#ifndef SERVO_H_
#define SERVO_H_



#include "stm32g4_timer.h" // <--- INDISPENSABLE pour timer_id_t
#include <stdint.h>

void SERVO_init(void);
void SERVO_set_position(timer_id_t timer_id, uint16_t TIM_CHANNEL_x, uint16_t position);
void SERVO_process_test(timer_id_t timer_id, uint16_t TIM_CHANNEL_x);
void SERVO_set_angle(timer_id_t timer, uint32_t channel, uint16_t angle);
uint16_t SERVO_get_position(void);


#endif /* SERVO_H_ */

