/**
 *******************************************************************************
 * @file	stm32g4_yx6300.c
 * @author	Samuel Poiraud 	&& Luc Hérault
 * @date	20xx 			&& 2024 --> portage sur stm32g431
 * @brief	Module pour utiliser le lecteur MP3 YX6300
 *******************************************************************************
 */

#include "config.h"
#if USE_YX6300
#include "stm32g4_yx6300.h"
#include "stm32g4_uart.h"


#ifndef YX6300_UART_ID
	#define YX6300_UART_ID	UART1_ID
#endif

#define PLAY_WITH_FOLDER_AND_FILE_NAME	0x0F //permet de choisir son document et sa musique
#define SET_VOLUME 0x06 //0x1E max (30 base 10)
//ces dernier sont à utilser dans les paramètres YX6300_send_request_with_2bytes_of_datas


#define PAUSE_MUSIQUE 0x0E //permet me mettre pause
#define PLAY_MUSIQUE 0x0D //permet me mettre pause
#define VOLUME_UP 0x04 //la différence est petite
#define VOLUME_DOWN 0x05 //la différence est petite
#define STOP_MUSIQUE 0x16 //ne peut être relancé
//ces dernier sont à utilser dans les paramètres YX6300_send_request


/*
#define NEXT_SONG 0x01 //fonctionne aussi sur la dernière musique (celui-ci le remet au début) attention ! il est possible que la prochaine chanson ne soit pas celle voulu
#define LAST_SONG 0x02 //jouer la musique d'avant
#define PLAY_WITH_INDEX 0x03 //pour jouer une musique
le résultat de ces commande laisse à désirer, mais elle fonctionne après vous pouvez tomber sur une musique non souhaité
*/

void BSP_YX6300_demo(void)
{

	HAL_Delay(1000);//attendre 1s pour éviter le mélange d'information

	uint8_t data[2];
	data[0] = 0x00;//numéro pour le volume (doit être à 0)
	data[1] = 0x06;//numéro de le volume (max 1E)
	BSP_YX6300_send_request_with_2bytes_of_datas(SET_VOLUME, false, 2, data);
	HAL_Delay(2000);
	data[0] = 0x05;//numéro du dossier choisie
	data[1] = 0x05;//numéro de la musique choisie
	BSP_YX6300_send_request_with_2bytes_of_datas(PLAY_WITH_FOLDER_AND_FILE_NAME, false, 2, data);
	HAL_Delay(10000);//attendre pour entendre la musique
	BSP_YX6300_send_request(PAUSE_MUSIQUE, false);
	HAL_Delay(5000);//attendre avant de reprendre la musique
	BSP_YX6300_send_request(PLAY_MUSIQUE, false);
	HAL_Delay(5000);
	for(int i=0; i<5; i++)
	{
	BSP_YX6300_send_request(VOLUME_DOWN, false);
	HAL_Delay(1000);
	}
	BSP_YX6300_send_request(STOP_MUSIQUE, false);
	HAL_Delay(2000);
}

void BSP_YX6300_send_request(uint8_t command, bool feedback)
{
	uint8_t msg[2+4+10];	//on fait le choix de refuser la demmande si datasize > 10
	uint8_t i = 0;
	msg[i++] = 0x7E;
	msg[i++] = 0xFF;
	msg[i++] = 2+4;	//length = (FF+length+command+feedback) + datasize
	msg[i++] = command;
	msg[i++] = (feedback)?1:0;
	
	msg[i++] = 0x00;
	msg[i++] = 0x00;
	
	msg[i++] = 0xEF;
	
	BSP_UART_puts(YX6300_UART_ID, msg, i);
}

void BSP_YX6300_send_request_with_2bytes_of_datas(uint8_t command, bool feedback, uint8_t datasize, uint8_t * data)
{
	uint8_t msg[2+4+10];	//on fait le choix de refuser la demmande si datasize > 10
	
	uint8_t i = 0;
	
	if(datasize<=10)
	{
		msg[i++] = 0x7E;
		msg[i++] = 0xFF;
		msg[i++] = 0x06;//datasize+4;	//length = (FF+length+command+feedback) + datasize
		msg[i++] = command;
		msg[i++] = (feedback)?1:0;

		uint8_t d;
		for(d=0; d<datasize; d++)
			msg[i++] = data[d];

		msg[i++] = 0xEF;

		BSP_UART_puts(YX6300_UART_ID, msg, i);
	}
	else
	{
		debug_printf("you should correct this function or respect the datasize limit!\n");
	}
}


#endif
