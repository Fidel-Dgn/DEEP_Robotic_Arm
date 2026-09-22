/**
 *******************************************************************************
 * @file	stm32g4_xpt2046.h
 * @author	vchav
 * @date	May 23, 2024
 * @brief	Module pour utiliser le tactile XPT2046 du TFT ili9341 sur cible stm32g4
 * 			Adaptation du module stm32f1_xpt2046.c de louisz
 *******************************************************************************
 */
#include "config.h"
#if USE_XPT2046

#ifndef BSP_LCD_ILI9341_STM32F1_XPT2046_H_
#define BSP_LCD_ILI9341_STM32F1_XPT2046_H_

#include "stm32g4_utils.h"

typedef enum{
	XPT2046_COORDINATE_RAW,
	XPT2046_COORDINATE_SCREEN_RELATIVE
}XPT2046_coordinateMode_e;

void XPT2046_init(void);
void XPT2046_demo(void);
bool XPT2046_getCoordinates(int16_t * pX, int16_t * pY, XPT2046_coordinateMode_e coordinateMode);
bool XPT2046_getMedianCoordinates(int16_t * pX, int16_t * pY, XPT2046_coordinateMode_e coordinateMode);
bool XPT2046_getAverageCoordinates(int16_t * pX, int16_t * pY, uint8_t nSamples, XPT2046_coordinateMode_e coordinateMode);

#ifndef XPT2046_SPI
	#define XPT2046_SPI           	SPI1
#endif

#define PIN_CS_TOUCH GPIOA, GPIO_PIN_11
#define PIN_IRQ_TOUCH GPIOB, GPIO_PIN_5

#define XPT2046_USE_PIN_IRQ_TO_CHECK_TOUCH

#endif // BSP_LCD_ILI9341_STM32F1_XPT2046_H_
#endif // USE_XPT2046
