/**
 *******************************************************************************
 * @file 	stm32g4_uart.c
 * @author 	jjo
 * @date 	Mar 29, 2024
 * @brief	Module UART pour cible stm32g4
 *******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BSP_STM32G4_UART_H_
#define BSP_STM32G4_UART_H_

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "stm32g4_utils.h"

/* Exported macros -----------------------------------------------------------*/
#if !((defined UART1_ON_PB7_PB6) ^ (defined UART1_ON_PA10_PA9))	//Un "non ou exclusif", ça c'est la classe !
	#error "Dans config.h -> vous devez choisir entre UART1_ON_PB6_PB7 ou UART1_ON_PA9_PA10."
#endif

#if !((defined UART2_ON_PA3_PA2) ^ (defined UART2_ON_PA15_PA14) ^ (defined UART2_ON_PB4_PB3))
	#error "Dans config.h -> vous devez choisir entre UART2_ON_PA2_PA3 ou UART2_ON_PA14_PA15 ou UART2_ON_PB3_PB4."
#endif

#define ESCAPE_KEY_CODE	0x1B

/* Exported types ------------------------------------------------------------*/
typedef enum
{
	UART1_ID = 0,
	UART2_ID,
	UART_ID_NB
}uart_id_t;

typedef void(*rx_callback_fun_t)(char c);	//Type pointeur sur fonction


/* Exported functions prototypes ---------------------------------------------*/
void BSP_UART_demo(void);

void BSP_UART_init(uart_id_t uart_id, uint32_t baudrate);

void BSP_UART_deinit(uart_id_t uart_id);

bool BSP_UART_button(uart_id_t uart_id);

void BSP_UART_putc(uart_id_t uart_id, uint8_t c);

uint8_t BSP_UART_get_next_byte(uart_id_t uart_id);

uint8_t BSP_UART_getc(uart_id_t uart_id);

uint8_t BSP_UART_getc_blocking(uart_id_t uart_id, uint32_t timeout_ms);

uint32_t BSP_UART_gets_blocking(uart_id_t uart_id, uint8_t * datas, uint32_t len, uint32_t timeout);

uint32_t BSP_UART_gets(uart_id_t uart_id, uint8_t * datas, uint32_t len);

void BSP_UART_puts(uart_id_t uart_id, const uint8_t *str, uint16_t len);

bool BSP_UART_data_ready(uart_id_t uart_id);

uint32_t BSP_UART_get_nb_data_ready(uart_id_t uart_id);

void BSP_UART_set_callback(uart_id_t uart_id, rx_callback_fun_t cb);

void BSP_UART_impolite_force_puts_on_uart(uart_id_t uart_id, uint8_t * str, uint32_t len);

#endif /* BSP_STM32G4_UART_H_ */

