/**
 ******************************************************************************
 * @file 	stm32g4_ws2812s.h
 * @author 	Samuel Poiraud 	&& Luc Hérault
 * @date 	May 3, 2016		&& 2024 --> portage sur G431
 * @brief 	Module pour controler le ws2812s
 ******************************************************************************
 */


#ifndef BSP_WS2812_STM32G4_WS2812_H_
#define BSP_WS2812_STM32G4_WS2812_H_

#include "config.h"
#if USE_WS2812


#define WS2812_COLOR_BLACK			0x000000
#define WS2812_COLOR_BLUE			0x0000FF
#define WS2812_COLOR_RED			0x00FF00
#define WS2812_COLOR_GREEN			0xFF0000
#define WS2812_COLOR_WHITE			0xFFFFFF
#define WS2812_COLOR_LIGHT_BLUE		0x000010
#define WS2812_COLOR_LIGHT_RED		0x001000
#define WS2812_COLOR_LIGHT_GREEN	0x100000
#define WS2812_COLOR_LIGHT_WHITE	0x102010


void BSP_WS2812_init(void);
void BSP_WS2812_demo(void);

void BSP_WS2812_display(uint32_t * pixels, uint8_t size); //pixels est un tableau de 64 cases maximum... (size est le nombre de cases)
void BSP_WS2812_display_only_one_pixel(uint32_t pixel, uint8_t rank, uint8_t size);
void BSP_WS2812_display_full(uint32_t pixel, uint8_t size);
void BSP_WS2812_send_pixel(uint32_t pixel);
void BSP_WS2812_reset(void);

#endif
#endif /* BSP_WS2812_STM32G4_WS2812_H_ */
