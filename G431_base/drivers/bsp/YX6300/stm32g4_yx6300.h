/**
 *******************************************************************************
 * @file	stm32g4_yx6300.h
 * @author	Samuel Poiraud && Luc Hérault
 * @date	2018 && 2024
 * @brief	Module pour utiliser le lecteur MP3 YX6300
 *******************************************************************************
 */

#ifndef YX6300_H
#define YX6300_H

#include "config.h"
#if USE_YX6300
#include <stdbool.h>
#include <stdint.h>


void BSP_YX6300_demo(void);

void BSP_YX6300_send_request(uint8_t command, bool feedback);

void BSP_YX6300_send_request_with_2bytes_of_datas(uint8_t command, bool feedback, uint8_t datasize, uint8_t * data);

#endif
#endif //YX6300_H
