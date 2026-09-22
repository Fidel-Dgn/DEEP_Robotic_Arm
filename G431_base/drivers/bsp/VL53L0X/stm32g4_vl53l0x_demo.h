/**
 *******************************************************************************
 * @file	stm32g4_vl53l0x_demo.h
 * @author	vchav
 * @date	May 31, 2024
 * @brief	Module visant à prendre en main le capteur de distance laser vl53l0x
 *******************************************************************************
 */
#ifndef BSP_VL53L0X_STM32G4_VL53L0X_DEMO_H_
#define BSP_VL53L0X_STM32G4_VL53L0X_DEMO_H_

#include "config.h"
#if USE_VL53L0

	#include "stm32g4_utils.h"

	typedef enum
	{
		TIMESLOT_ASK_BEGIN = 0,
		TIMESLOT_ASK_END = VL53_NB-1,
		TIMESLOT_WAIT_TIME_DURING_MEASUREMENTS = 21,
		//TIMESLOT_GET_BEGIN = 21,
		//TIMESLOT_GET_END = 21+VL53_NB-1,
		TIMESLOT_NB
	}timeslot_e;

	void VL53L0X_Demo(void);

	bool VL53L0X_init(void);

	timeslot_e VL53L0X_process_1ms(void);

	void VL53L0X_process_main(void);

	uint16_t VL53L0X_get_distance(uint8_t id);

#endif

#endif /* BSP_VL53L0X_STM32G4_VL53L0X_DEMO_H_ */
