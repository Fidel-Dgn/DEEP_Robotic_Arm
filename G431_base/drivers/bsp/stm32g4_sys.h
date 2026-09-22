/**
 *******************************************************************************
 * @file 	stm32g4_sys.c
 * @author 	jjo
 * @date 	Mar 11, 2024
 * @brief 	Module d'initialisation et de gestion d'erreur
 *******************************************************************************
 */

#ifndef BSP_STM32G4_SYS_H_
#define BSP_STM32G4_SYS_H_

/* Includes ------------------------------------------------------------------*/
#include "stm32g4_uart.h"

/* Exported functions prototypes ---------------------------------------------*/
void BSP_SYS_set_std_usart(uart_id_t in, uart_id_t out, uart_id_t err);
void Error_Handler(void);
void SystemClock_Config(void);
void BSP_SYS_HSI48_Init(void);
uint32_t dump_printf(const char *format, ...);
void Delay_us(uint32_t us);	//fonction disponible en assembleur dans startup.s

#endif /* BSP_STM32G4_SYS_H_ */
