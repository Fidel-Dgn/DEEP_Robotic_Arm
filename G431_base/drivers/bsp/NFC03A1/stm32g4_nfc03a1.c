/**
 *******************************************************************************
 * @file	stm32g4_nfc03a1.c
 * @author	Julie			&& vchav
 * @date	1 janv. 2017	&& Jul 9, 2024 --> portage sur stm32g431
 * @brief	Module pour utiliser le NFC03A1
 *******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#if USE_NFC03A1
#include "stdint.h"
#include "stdbool.h"
#include "stm32g4_nfc03a1.h"
#include "stm32g4_gpio.h"
#include "stm32g4_extit.h"
#include <stdio.h>

static bool initialized = false;
extern DeviceMode_t devicemode;

/* Code exemple du lecteur NFC03A1 : voir fichier NFC_example.txt--------------*/

/*
 * 1 - Inclure le header de la librairie NFC avec son chemin complet :
 *
 *		#include "NFC03A1/nfc03a1.h"
 */

/*
 * 2 - Déclarer le mode d'utilisation du module NFC, 2 modes sont disponibles
 * 	   dans cette librairie : Proximity Coupling Device (PCD), mode de lecture/
 * 	   écriture, et Proximity Inductive Coupling Card (PICC), mode d'émulation
 * 	   de tags.
 *
 *		extern DeviceMode_t devicemode;
 */


/* Private variables ---------------------------------------------------------*/

/* TT1 (PCD only)*/
uint8_t TT1Tag[NFCT1_MAX_TAGMEMORY];

/* TT2 */
uint8_t TT2Tag[NFCT2_MAX_TAGMEMORY];

/* TT3 */
uint8_t TT3Tag[NFCT3_MAX_TAGMEMORY];
uint8_t *TT3AttribInfo = TT3Tag, *TT3NDEFfile = &TT3Tag[NFCT3_ATTRIB_INFO_SIZE];

/* TT4 */
uint8_t CardCCfile      [NFCT4_MAX_CCMEMORY];
uint8_t CardNDEFfileT4A [NFCT4_MAX_NDEFMEMORY];
uint8_t CardNDEFfileT4B [NFCT4_MAX_NDEFMEMORY];

/* TT5 (PCD only)*/
uint8_t TT5Tag[NFCT5_MAX_TAGMEMORY];




//dm : PCD or PICC
void BSP_NFC03A1_Init(DeviceMode_t dm)
{
	devicemode = dm;
	/* 95HF HW Init */
	BSP_GPIO_pin_config(NFC_INTERFACE_GPIO_PORT, NFC_INTERFACE_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
	#if USE_NFC_WITH_UART
		/* Set interface pin to select UART */
		HAL_GPIO_WritePin(NFC_INTERFACE_GPIO_PORT, NFC_INTERFACE_PIN, GPIO_PIN_RESET);
	#endif
	#if USE_NFC_WITH_SPI
		/* Set interface pin to select SPI */
		HAL_GPIO_WritePin(NFC_INTERFACE_GPIO_PORT, NFC_INTERFACE_PIN, GPIO_PIN_SET);
	#endif /*  */

	ConfigManager_HWInit();

	initialized = true;
}

//Fonction de test, blocante.
void BSP_NFC03A1_demo(void)
{
	uint8_t tag;
	if(!initialized)
		BSP_NFC03A1_Init(PCD);

	while(1)
	{
		tag = ConfigManager_TagHunting(TRACK_ALL);
		switch(tag)
		{
			case TRACK_NFCTYPE4A:{
				ISO14443A_CARD infos;
				BSP_NFC03A1_get_ISO14443A_infos(&infos);
				uint8_t i;
				printf("uid = ");
				for(i=0; i<infos.UIDsize;i++)
					printf("%02x ",infos.UID[i]);
				printf("\n");
				break;}
			default:
				break;
		}
	}

}



/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define CURSOR_STEP             5

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

volatile bool uDelayUs;
volatile bool uTimeOut;
volatile bool uAppliTimeOut;
volatile bool terminal_display_enabled = 0;

bool uDataReady 				= false;
bool RF_DataExpected 			= false;
bool RF_DataReady 				= false;

uint16_t delay_micros           = 0;
uint16_t delay_timeout          = 0;
uint16_t delay_appli            = 0;
uint32_t nb_ms_elapsed          = 0;

extern ISO14443A_CARD ISO14443A_Card;
void BSP_NFC03A1_get_ISO14443A_infos(ISO14443A_CARD * infos)
{
	*infos = ISO14443A_Card;
}





/**
 * @brief  EXTI line detection callbacks
 * @param  GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */

void RFTRANS_95HF_EXTI_Callback(uint8_t GPIO_Pin)
{
  if(GPIO_Pin == BSP_EXTIT_gpiopin_to_pin_number(IRQOUT_RFTRANS_95HF_PIN))
  {
    if(RF_DataExpected)
      RF_DataReady = true;

    /* Answer to command ready*/
    uDataReady = true;
  }
}



/**
 *	@brief  This function configures the Extern Interrupt for the IRQ coming
 *              from the RF transceiver
 */
void drvInt_Enable_Reply_IRQ(void)
{
  RF_DataExpected = false;
  uDataReady = false;
  BSP_EXTIT_enable(BSP_EXTIT_gpiopin_to_pin_number(IRQOUT_RFTRANS_95HF_PIN));
}

/**
 *	@brief  This function configures the Extern Interrupt for the IRQ coming
 *      from the RF transceiver
 */
void drvInt_Enable_RFEvent_IRQ(void)
{
  RF_DataExpected = true;
  uDataReady = false;
  BSP_EXTIT_enable(BSP_EXTIT_gpiopin_to_pin_number(IRQOUT_RFTRANS_95HF_PIN));
}

/**
 *	@brief  This function configures the Extern Interrupt for the IRQ coming
 *      from the RF transceiver
 */
void drvInt_Disable_95HF_IRQ(void)
{
  RF_DataExpected = false;
  uDataReady = false;
  BSP_EXTIT_disable(BSP_EXTIT_gpiopin_to_pin_number(IRQOUT_RFTRANS_95HF_PIN));
}

#endif

