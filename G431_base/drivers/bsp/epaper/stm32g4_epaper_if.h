/**
 *******************************************************************************
 * @file	stm32g4_epaper_if.h
 * @author	vchav
 * @date	May 29, 2024
 *******************************************************************************
 * @brief	Header file of stm32g4_epaper_if.c providing EPD interface functions
 * @author	Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     July 7 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "config.h"
#if USE_EPAPER
#ifndef BSP_EPAPER_STM32G4_EPAPER_IF_H_
#define BSP_EPAPER_STM32G4_EPAPER_IF_H_

#include "stm32g4xx_hal.h"

// Pin definition
#define CS_PIN           0
#define RST_PIN          1
#define DC_PIN           2
#define BUSY_PIN         3

// Pin level definition
#define LOW             0
#define HIGH            1

#define RST_Pin 			GPIO_PIN_3
#define RST_GPIO_Port 		GPIOB

#define DC_Pin 				GPIO_PIN_8
#define DC_GPIO_Port 		GPIOA

#define BUSY_Pin 			GPIO_PIN_0
#define BUSY_GPIO_Port 		GPIOB

#define SPI_CS_Pin 			GPIO_PIN_4
#define SPI_CS_GPIO_Port 	GPIOA

#define EPAPER_SPI			SPI1

typedef struct {
  GPIO_TypeDef* port;
  int pin;
} EPD_Pin;

int  EpdInitCallback(void);
void EpdDigitalWriteCallback(int pin, int value);
int  EpdDigitalReadCallback(int pin);
void EpdDelayMsCallback(unsigned int delaytime);
void EpdSpiTransferCallback(unsigned char data);


#endif /* USE_EPAPER */
#endif /* BSP_EPAPER_STM32G4_EPAPER_IF_H_ */
