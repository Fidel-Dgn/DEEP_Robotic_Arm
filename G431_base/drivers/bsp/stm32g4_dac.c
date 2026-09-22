/**
 *******************************************************************************
 * @file	stm32g4_dac.c
 * @author	vchav
 * @date	Jun 7, 2024
 * @brief	Module pour utiliser le DAC (Digital to Analog Converter) sur cible stm32g4
 *******************************************************************************
 */


/*
 * -----------------------------------------------------------------------------------------------------------
 *
 * Dans ce module, vous avez le choix entre 2 canaux pour utiliser le DAC :
 * 	- Le canal 1 (en PA4)
 * 	- Le canal 2 (en PA5)
 * L'emplacement hardware de ces canaux sous-entend de voler des GPIO à un possible TFT.
 * Si vous souhaitez utiliser l'écran TFT, vous devrez donc surement changer une ou deux de ses pins.
 *
 * -----------------------------------------------------------------------------------------------------------
 *
 * Pour le DAC, 2 modes sont proposés: le mode Normal et le mode Sample-And-Hold.
 *
 * SAMPLE_AND_HOLD:
 * Pour mieux comprendre ce que permet ce mode (et donc s'il est pertinent pour votre projet), voici une explication de notre cher GPT:
 *
 * Le mode "Sample and Hold" (échantillonnage et maintien) sur le DAC d'un microcontrôleur STM32, tel que le STM32G431,
 * est une fonctionnalité qui permet de conserver la sortie analogique du DAC stable pendant une période de temps spécifique,
 * même si l'entrée numérique change.
 *
 * Fonctionnement du Mode Sample and Hold:
 *
 * 		1. Échantillonnage (Sample) :
 * 			- Le DAC convertit la valeur numérique donnée en une tension analogique.
 * 			- Cette tension analogique est capturée et stockée sur un condensateur interne.
 *
 * 		2. Maintien (Hold) :
 * 			- Après la phase d'échantillonnage, le condensateur interne maintient la tension analogique à une valeur constante
 * 			  pendant une certaine période de temps, même si les valeurs numériques changent.
 * 			- Cela permet de stabiliser la sortie analogique et d'éviter des changements brusques de la tension de sortie.
 *
 * Avantages:
 * 		- Une sortie stable (bien ou pas selon votre projet)
 * 		- Un réduction du bruit
 *
 *
 * Sinon, vous pouvez simplement mettre le mode Normal
 */

#include "config.h"
#if USE_DAC

#include "stm32g4_dac.h"
#include "stm32g4_utils.h"
#include "stm32g4_sys.h"
#include <math.h>
#include <stm32g4xx_hal_dac.h>


DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac1_ch1;
DMA_HandleTypeDef hdma_dac1_ch2;

void DAC_Init_port(dac_out_e outx);
void DAC_Init_dma(dac_out_e outx);
void generate_sine_wave(void);

/**
 * @brief Initialise le DAC
 * @param out: DAC1_OUT1(0) ou DAC1_OUT2(1) en fonction du canal que l'on souhaite utiliser
 * @param mode: DAC_MODE_NORMAL ou DAC_MODE_SAMPLE_AND_HOLD (voir explication au dessus)
 * @param with_dma: true si on veut utiliser le DMA, false sinon.
 * @note si vous souhaitez utiliser le mode SAMPLE_AND_HOLD, vous devrez surement modifier les valeurs de ces 3 paramètres:
 * 			- sConfig.DAC_SampleAndHoldConfig.DAC_SampleTime
 * 			- sConfig.DAC_SampleAndHoldConfig.DAC_HoldTime
 * 			- sConfig.DAC_SampleAndHoldConfig.DAC_RefreshTime
 */
void BSP_DAC_Init(dac_out_e outx, dac_mode_e mode, bool with_dma){

	DAC_ChannelConfTypeDef sConfig = {0};

	/* DAC Initialization */
	hdac.Instance = DAC1;
	if (HAL_DAC_Init(&hdac) != HAL_OK)
	{
	Error_Handler();
	}

	DAC_Init_port(outx);

	/* DAC channel OUTx config */
	sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
	sConfig.DAC_DMADoubleDataMode = DISABLE;
	sConfig.DAC_SignedFormat = DISABLE;
	if(mode)
		sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
	else{
		sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_ENABLE;
		sConfig.DAC_SampleAndHoldConfig.DAC_SampleTime = 1023; 	// between 0 and 1023
		sConfig.DAC_SampleAndHoldConfig.DAC_HoldTime = 1023; 	// between 0 and 1023
		sConfig.DAC_SampleAndHoldConfig.DAC_RefreshTime = 255; 	// between 0 and 255
	}
	sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
	sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
	sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_EXTERNAL;
	sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;

	if(outx)
		HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2);
	else
		HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);

	if(with_dma){
		DAC_Init_dma(outx);
	}
}

/**
 * @brief Initialise le GPIO en fonction du canal souhaité
 * @param out: DAC1_OUT1(0) ou DAC1_OUT2(1) en fonction du canal que l'on souhaite utiliser
 */
void DAC_Init_port(dac_out_e outx){

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if(hdac.Instance==DAC1){

	/* DAC1 clock enable */
	__HAL_RCC_DAC1_CLK_ENABLE();

	__HAL_RCC_GPIOA_CLK_ENABLE();
	/*DAC1 GPIO Configuration
	PA4     ------> DAC1_OUT1
	PA5     ------> DAC1_OUT2
	*/
	if(outx)
		GPIO_InitStruct.Pin = GPIO_PIN_5;
	else
		GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
}

/**
 * @brief Initialise et lance le DMA
 * @param out: DAC1_OUT1(0) ou DAC1_OUT2(1) en fonction du canal que l'on souhaite utiliser
 */
void DAC_Init_dma(dac_out_e outx){

	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_DMAMUX1_CLK_ENABLE();

	if(outx){
		hdma_dac1_ch2.Instance = DMA1_Channel2;
		hdma_dac1_ch2.Init.Request = DMA_REQUEST_DAC1_CHANNEL2;
		hdma_dac1_ch2.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hdma_dac1_ch2.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_dac1_ch2.Init.MemInc = DMA_MINC_ENABLE;
		hdma_dac1_ch2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
		hdma_dac1_ch2.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
		hdma_dac1_ch2.Init.Mode = DMA_CIRCULAR;
		hdma_dac1_ch2.Init.Priority = DMA_PRIORITY_HIGH;
		if (HAL_DMA_Init(&hdma_dac1_ch2) != HAL_OK)
		  Error_Handler();

		__HAL_LINKDMA(&hdac,DMA_Handle2,hdma_dac1_ch2);

		HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

	}else{
	    hdma_dac1_ch1.Instance = DMA1_Channel1;
	    hdma_dac1_ch1.Init.Request = DMA_REQUEST_DAC1_CHANNEL1;
	    hdma_dac1_ch1.Init.Direction = DMA_MEMORY_TO_PERIPH;
	    hdma_dac1_ch1.Init.PeriphInc = DMA_PINC_DISABLE;
	    hdma_dac1_ch1.Init.MemInc = DMA_MINC_ENABLE;
	    hdma_dac1_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	    hdma_dac1_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	    hdma_dac1_ch1.Init.Mode = DMA_CIRCULAR;
	    hdma_dac1_ch1.Init.Priority = DMA_PRIORITY_HIGH;
	    if (HAL_DMA_Init(&hdma_dac1_ch1) != HAL_OK)
	      Error_Handler();

	    __HAL_LINKDMA(&hdac,DMA_Handle1,hdma_dac1_ch1);

	    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	}
	HAL_NVIC_SetPriority(DMAMUX_OVR_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMAMUX_OVR_IRQn);
}

/**
 * @brief Lance le DAC sans le DMA
 * @param outx: Le canal de sortie du DAC: OUT1 ou OUT2 (voir le fichier .h ou en haut de la page).
 */
void BSP_DAC_Start_without_dma(dac_out_e outx){
	if (HAL_DAC_Start(&hdac, outx?DAC_CHANNEL_2:DAC_CHANNEL_1) != HAL_OK)
		Error_Handler();
}

/**
 * @brief  Démarre le DAC en utilisant le DMA.
 * @param  outx: Le canal de sortie du DAC: OUT1 ou OUT2 (voir le fichier .h ou en haut de la page).
 * @param  dac_buffer: Pointeur vers le buffer/tableau contenant les valeurs numériques à convertir en analogique.
 * @param  size: Taille du buffer. Spécifie le nombre d'éléments dans le buffer à convertir.
 * @note   Cette fonction utilise le DMA pour transférer les données de la mémoire vers le périphérique DAC.
 *         Assurez-vous d'avoir bien initilialisé les périphériques DMA et DAC avant d'appeler cette fonction.
 */
void BSP_DAC_Start_with_dma(dac_out_e outx, uint32_t* dac_buffer, uint32_t size)
{
    if (HAL_DAC_Start_DMA(&hdac, outx?DAC_CHANNEL_2:DAC_CHANNEL_1, dac_buffer, size, DAC_ALIGN_12B_R) != HAL_OK)
        Error_Handler();
}

/**
 * @brief  Si vous utilisez le DAC sans le DMA, cette fonction est faite pour vous.
 *         Elle permet d'envoyer une valeur au DAC.
 * @param outx: Canal utilisé, OUT1 ou OUT2 (voir le fichier .h ou en haut de la page).
 * @param value: Valeur à convertir.
 * @return true si tout s'est bien déroulé, false sinon.
 */
bool BSP_DAC_Set_value(dac_out_e outx, uint32_t value){
	if(value > 0xFF)
		value = 0xFF;

	if(HAL_DAC_SetValue(&hdac, outx?DAC_CHANNEL_2:DAC_CHANNEL_1, DAC_ALIGN_12B_R, value) != HAL_OK)
		return false;

	return true;
}

uint32_t sine_wave[100];
// Fonction pour générer une sinusoïde
void generate_sine_wave(void) {
    for (int i = 0; i < 100; i++) {
        sine_wave[i] = (uint32_t)((sin(2 * 3.14 * i / 100) + 1) * 2048); // Convert to 12-bit DAC value
    }
}

/**
 * @brief Fonction de démonstration sans l'utilisation du DMA.
 * @pre /!\ Cette fonction est blocante
 * @note Pour constater ce qui se passe dans la fonction, il est conseillé de sonder à l'oscilloscope la broche PA5.
 *       Avec les bons réglages, vous devriez voir une sinusoïde.
 *       Sinon, vous pouvez toujours brancher une LED et constater que son intensité varie rapidement.
 */
void BSP_DAC_demo_without_dma(void) {
    BSP_DAC_Init(DAC1_OUT2, DAC_MODE_NORMAL, false);
    BSP_DAC_Start_without_dma(DAC1_OUT2);

    generate_sine_wave();
    while (1) {
        for (int i = 0; i < 100; i++) {
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, sine_wave[i]);
            HAL_Delay(1); // Délai pour contrôler la fréquence de l'onde
        }
    }
}


/**
 * @brief Fonction de démonstration avec l'utilisation du DMA.
 * @pre /!\ Cette fonction est blocante
 * @note Pour constater ce qui se passe dans la fonction, il est conseillé de sonder à l'oscilloscope la broche PA5.
 *       Avec les bons réglages, vous devriez voir une sinusoïde.
 *       Sinon, vous pouvez toujours brancher une LED et constater que son intensité varie rapidement.
 */
// /!\ Ne fonctionne pas encore
void BSP_DAC_demo_with_dma() {
    // Initialisation du DAC avec DMA
    BSP_DAC_Init(DAC1_OUT1, DAC_MODE_NORMAL, true); // Initialise le DAC1 avec le mode normal et le DMA activé

    generate_sine_wave();

    // Démarrage du DAC avec DMA
    BSP_DAC_Start_with_dma(DAC1_OUT1, sine_wave, 100);

    // Attente infinie
    while (1) {}
}

void DMA1_Channel1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_dac1_ch1);
}

void DMA1_Channel2_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_dac1_ch2);
}

void DMAMUX_OVR_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_dac1_ch1); // Ou hdma_dac1_ch2 en fonction du canal
}

#endif
