/**
 *******************************************************************************
 * @file	stm32g4_epaper_if.c
 * @author	vchav
 * @date	May 29, 2024
 *******************************************************************************
 * @brief	Implements EPD interface functions
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
#include "stm32g4_epaper_if.h"
#include "stm32g4_spi.h"
#include "stm32g4_gpio.h"

EPD_Pin epd_cs_pin = {
  SPI_CS_GPIO_Port,
  SPI_CS_Pin,
};

EPD_Pin epd_rst_pin = {
  RST_GPIO_Port,
  RST_Pin,
};

EPD_Pin epd_dc_pin = {
  DC_GPIO_Port,
  DC_Pin,
};

EPD_Pin epd_busy_pin = {
  BUSY_GPIO_Port,
  BUSY_Pin,
};

EPD_Pin pins[4];

/**
 * @brief Écrit une valeur numérique sur un pin.
 * @param pin_num: Numéro du pin.
 * @param value: Valeur à écrire (HIGH ou LOW).
 */
void EpdDigitalWriteCallback(int pin_num, int value) {
  if (value == HIGH) {
    HAL_GPIO_WritePin((GPIO_TypeDef*)pins[pin_num].port, pins[pin_num].pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin((GPIO_TypeDef*)pins[pin_num].port, pins[pin_num].pin, GPIO_PIN_RESET);
  }
}

/**
 * @brief Lit une valeur numérique à partir d'un pin.
 * @param pin_num: Numéro du pin.
 * @return La valeur lue (HIGH ou LOW).
 */
int EpdDigitalReadCallback(int pin_num) {
  if (HAL_GPIO_ReadPin(pins[pin_num].port, pins[pin_num].pin) == GPIO_PIN_SET) {
    return HIGH;
  } else {
    return LOW;
  }
}

/**
 * @brief Fait une pause pour un certain temps en millisecondes.
 * @param delaytime: Temps de pause en millisecondes.
 */
void EpdDelayMsCallback(unsigned int delaytime) {
  HAL_Delay(delaytime);
}

/**
 * @brief Transfère un octet de données via SPI.
 * @param data: Donnée à transférer.
 */
void EpdSpiTransferCallback(unsigned char data) {
  HAL_GPIO_WritePin((GPIO_TypeDef*)pins[CS_PIN].port, pins[CS_PIN].pin, GPIO_PIN_RESET);
  BSP_SPI_WriteNoRegister(EPAPER_SPI, data);
  HAL_GPIO_WritePin((GPIO_TypeDef*)pins[CS_PIN].port, pins[CS_PIN].pin, GPIO_PIN_SET);
}

/**
 * @brief Initialise les pins et SPI pour l'afficheur epaper.
 * @return 0 si l'initialisation est réussie.
 */
int EpdInitCallback(void) {
	BSP_GPIO_pin_config(epd_cs_pin.port, epd_cs_pin.pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH,GPIO_NO_AF);
	BSP_GPIO_pin_config(epd_dc_pin.port, epd_dc_pin.pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH,GPIO_NO_AF);
	BSP_GPIO_pin_config(epd_rst_pin.port, epd_rst_pin.pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH,GPIO_NO_AF);
	BSP_GPIO_pin_config(epd_busy_pin.port, epd_busy_pin.pin, GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH,GPIO_NO_AF);
	pins[CS_PIN] = epd_cs_pin;
	pins[RST_PIN] = epd_rst_pin;
	pins[DC_PIN] = epd_dc_pin;
	pins[BUSY_PIN] = epd_busy_pin;
	BSP_SPI_Init(EPAPER_SPI, FULL_DUPLEX, MASTER, SPI_BAUDRATEPRESCALER_128);
	return 0;
}
#endif

