/**
 *******************************************************************************
 * @file	stm32g4_ds18b20.h
 * @author	Nirgal 		&& Luc Hérault
 * @date	3 mai 2021 	&& Juin 2024 --> portage sur g431
 * @brief	Module pour utiliser le DS18B20
 *******************************************************************************
 */

#ifndef BSP_DS18B20_H_
#define BSP_DS18B20_H_

#include "config.h"
#if USE_DS18B20

void BSP_DS18B20_init(void);
void BSP_DS18B20_demo(void);

int16_t BSP_DS18B20_get_temperature(void);
uint8_t BSP_DS18B20_Start (void); //enlèvement du stactic


#endif
#endif /* BSP_DS18B20_H_ */
