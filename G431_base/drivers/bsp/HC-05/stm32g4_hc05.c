/*
 * stm32g4_hc05.c
 *
 *  Created on: Sep 30, 2024
 *      Author: Nirgal
 */

#include "stm32g4_uart.h"


/*
 * Cette fonction est utilisée pour permettre le dialogue en un PC relié via la port série virtuel de la sonde de débogage (via l'UART2)
 *   et un module HC-05 relié à l'UART1 (PA9, PA10).
 *
 *  Son usage est généralement temporaire, le temps de la configuration du module HC-05 en mode AT.
 *
 *  Cette fonction doit être appelée en début de main() après HAL_init();
 *  Elle est blocante !
 */
void HC05_set_echo_for_AT_mode(void)
{
	BSP_UART_init(UART1_ID, 38400);		//vitesse nécessaire pour la configuration du HC-05 en mode AT
	BSP_UART_init(UART2_ID, 115200);
	uint8_t c;
	volatile bool config_mode = true;
	while(config_mode)
	{
		if(BSP_UART_data_ready(UART1_ID))
		{
			c = BSP_UART_getc(UART1_ID);
			BSP_UART_putc(UART2_ID, c);
		}
		if (BSP_UART_data_ready(UART2_ID))
		{
			c = BSP_UART_getc(UART2_ID);
			BSP_UART_putc(UART1_ID, c);
		}

		//pour sortir de cette boucle, il suffit de passer config_mode à false à l'aide du débogueur.
	}

	BSP_UART_init(UART1_ID, 115200);	//vitesse du HC-05 en mode de communication normale (si vous avez configuré cette vitesse ainsi).
}
