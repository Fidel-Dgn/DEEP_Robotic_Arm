/**
 *******************************************************************************
 * @file	stm32g4_dht11.h
 * @author	vchav
 * @date	Jun 12, 2024
 * @brief	Module pour utiliser le DHT11 (adaptation du module créé pour f103)
 *******************************************************************************
 */
#ifndef BSP_DHT11_STM32G4_DHT11_H_
#define BSP_DHT11_STM32G4_DHT11_H_
#include "config.h"
#if USE_DHT11
#include "stm32g4_utils.h"

void BSP_DHT11_demo(void);
void BSP_DHT11_init(GPIO_TypeDef * GPIOx, uint16_t GPIO_PIN_x);
running_t BSP_DHT11_state_machine_get_datas(uint8_t * humidity_int, uint8_t * humidity_dec, uint8_t * temperature_int, uint8_t * temperature_dec);

#endif
#endif /* BSP_DHT11_STM32G4_DHT11_H_ */
