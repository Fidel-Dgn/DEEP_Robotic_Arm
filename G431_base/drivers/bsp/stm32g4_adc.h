/**
 *******************************************************************************
 * @file	stm32g4_adc.h
 * @author	vchav
 * @date	May 23, 2024
 * @brief	
 *******************************************************************************
 */
#ifndef BSP_STM32G4_ADC_H_
#define BSP_STM32G4_ADC_H_

#include "config.h"

#if USE_ADC

	#include "stm32g4_utils.h"

	/**
	 * @brief Enumération des convertisseurs analogique numérique sélectionnable
	 */
	typedef enum{
		ADC_1 = 0,		//Entrée PA0
		ADC_2,			//Entrée PA1
		ADC_3,			//Entrée PA6
		ADC_4,			//Entrée PA7
		ADC_10,			//Entrée PF1
		ADC_13,			//Entrée PA5
		ADC_17,			//Entrée PA4
		ADC_CHANNEL_NB
	}adc_id_e;



	void DEMO_adc_statemachine (void);
	void BSP_ADC_init();
	uint16_t BSP_ADC_getValue(adc_id_e channel);
	bool BSP_ADC_is_new_sample_available(void);
	void BSP_ADC_set_callback_function(callback_fun_t callback);
	void DEMO_adc_process_1ms(void);

#endif //USE_ADC

#endif /* BSP_STM32G4_ADC_H_ */
