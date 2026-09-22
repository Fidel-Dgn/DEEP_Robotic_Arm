/**
 *******************************************************************************
 * @file	stm32g4_ld19_display.h
 * @author	vchav
 * @date	Jun 14, 2024
 * @brief	Module pour afficher les trames reçus du capteur lidar ld19
 *******************************************************************************
 */
#ifndef BSP_LD19_STM32G4_LD19_DISPLAY_H_
#define BSP_LD19_STM32G4_LD19_DISPLAY_H_
#include "config.h"
#if USE_LD19
#include "stm32g4_ld19.h"

void BSP_LD19_init_tft(void);
void BSP_LD19_display_on_tft(ld19_frame_handler_t *frame);

#endif
#endif /* BSP_LD19_STM32G4_LD19_DISPLAY_H_ */
