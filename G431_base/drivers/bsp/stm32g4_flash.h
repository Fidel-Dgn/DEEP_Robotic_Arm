/**
 *******************************************************************************
 * @file	stm32g4_flash.h
 * @author	vchav
 * @date	Jun 11, 2024
 * @brief	Adaptation du module créé par Samuel Poiraud pour la stm32f103rbt6
 *******************************************************************************
 */
#ifndef BSP_STM32G4_FLASH_H_
#define BSP_STM32G4_FLASH_H_

#include "stm32g4_sys.h"

uint64_t BSP_FLASH_read_doubleword(uint32_t index);
void BSP_FLASH_set_doubleword(uint32_t index, uint64_t data);
void BSP_FLASH_dump(void);
void FLASH_demo(void);

#endif /* BSP_STM32G4_FLASH_H_ */
