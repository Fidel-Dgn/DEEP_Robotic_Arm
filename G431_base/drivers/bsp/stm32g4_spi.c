/**
 ******************************************************************************
 * @file 	stm32g4_spi.c
 * @author 	vchav
 * @date 	May 14, 2024
 * @brief 	Module SPI pour cible stm32g4
 * 			Adaptation du module stm32f1_spi.c de Nirgal
 ******************************************************************************
 */

/*
 * Ce module permet d'Initialiser un bus SPI, Recevoir et Transmettre des données via ce SPI.
 *
 * La stm32g4 dispose de 3 sorties SPI
 *
 * 		SPI1 GPIO Configuration: 		|			SPI2 GPIO Configuration:				|			SPI3 GPIO Configuration:
 * 		PA5	------> SPI1_SCK			|			PF1-OSC_OUT ------> SPI2_SCK			|			PB3	------> SPI3_SCK
 * 		PA6	------> SPI1_MISO			|			PA10     	------> SPI2_MISO			|			PB4	------> SPI3_MISO
 * 		A7	------> SPI1_MOSI			|			PA11     	------> SPI2_MOSI			|			PB5	------> SPI3_MOSI
 *
 * 		/!\ PF1 n'est accessible que si SB11 est soudé et pas SB10 (petits pads à souder/relier sur la carte stm32g431) /!\
 *
 */
#include "stm32g4_spi.h"
#include "stm32g4_gpio.h"
#include "stm32g4_utils.h"
#include <assert.h>

typedef enum
{
	SPI1_ID = 0,
	SPI2_ID,
	SPI3_ID,
	SPI_NB
}SPI_ID_e;


void SPI_GPIO_FULLDUPLEX_config(SPI_ID_e SPI_id);
void SPI_GPIO_HALFDUPLEX_or_TRANSMITONLY_config(SPI_ID_e SPI_id);
void SPI_GPIO_RECEIVEONLY_config(SPI_ID_e SPI_id);
void SPI_Cmd(SPI_TypeDef* SPIx, FunctionalState NewState);


static SPI_HandleTypeDef  hSPI[SPI_NB];


/**
 * @brief Cette fonction initialise le bus SPI en fonction des 3 paramètres
 * @param SPIx: SPI1, SPI2 ou SPI3
 * @param SPI_Mode: Le mode de communication (FULL_DUPLEX, HALF_DUPLEX, ...)
 * @param SPI_Rank: MASTER si la stm32g4 est maitre ou SLAVE si elle est esclave
 * @param SPI_bp: Le Baud Rate prescaler utilisé pour configurer la clock SCK0.
 * @pre Si vous souhaitez configurer le SPI2, vous devez vous assurer que le pin F1 est actif en hardware (cf en haut de la page /!\..../!\ ou le tableau des pins)
 * @note /!\ Le baud Rate prescaler va dépendre du capteur que vous utilisé ;
 * 		 Ce paramètre est crucial et doit être réfléchi si vous voulez avoir une communication fonctionnelle et optimale /!\
 */
void BSP_SPI_Init(SPI_TypeDef* SPIx, SPI_Mode_e SPI_Mode, SPI_Rank_e SPI_Rank, uint16_t SPI_BAUDRATEPRESCALER_x)
{
	assert(SPIx == SPI1 || SPIx == SPI2 || SPIx == SPI3);
	assert(SPI_Mode == FULL_DUPLEX || SPI_Mode == HALF_DUPLEX || SPI_Mode == RECEIVE_ONLY || SPI_Mode == TRANSMIT_ONLY);
	assert(SPI_Rank == MASTER || SPI_Rank == SLAVE);
	assert(IS_SPI_BAUDRATE_PRESCALER(SPI_BAUDRATEPRESCALER_x));

	SPI_ID_e id = ((SPIx == SPI1)?SPI1_ID:(SPIx == SPI2)?SPI2_ID:SPI3_ID);

	hSPI[id].Instance = SPIx;
	hSPI[id].Init.DataSize = SPI_DATASIZE_8BIT;
	hSPI[id].Init.CLKPolarity = SPI_POLARITY_LOW;
	hSPI[id].Init.CLKPhase = SPI_PHASE_1EDGE;
	hSPI[id].Init.NSS = SPI_NSS_SOFT;			//Chip select must be manage by software
	hSPI[id].Init.FirstBit = SPI_FIRSTBIT_MSB;
	hSPI[id].Init.TIMode = SPI_TIMODE_DISABLE;
	hSPI[id].Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hSPI[id].Init.CRCPolynomial = 0;
	hSPI[id].Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	switch (SPI_Rank) {
		case MASTER:
			hSPI[id].Init.Mode = SPI_MODE_MASTER;
			hSPI[id].Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_x;
			hSPI[id].Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
			break;
		case SLAVE:
			hSPI[id].Init.Mode = SPI_MODE_SLAVE;
			hSPI[id].Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
			break;
		default:
			break;
	}
	switch (SPI_Mode) {
		case FULL_DUPLEX:
			hSPI[id].Init.Direction = SPI_DIRECTION_2LINES;
			SPI_GPIO_FULLDUPLEX_config(id);
			break;
		case HALF_DUPLEX:
			hSPI[id].Init.Direction = SPI_DIRECTION_1LINE;
			SPI_GPIO_HALFDUPLEX_or_TRANSMITONLY_config(id);
			break;
		case RECEIVE_ONLY:
			hSPI[id].Init.Direction = SPI_DIRECTION_2LINES_RXONLY;
			SPI_GPIO_RECEIVEONLY_config(id);
			break;
		case TRANSMIT_ONLY:
			hSPI[id].Init.Direction = SPI_DIRECTION_2LINES;
			SPI_GPIO_HALFDUPLEX_or_TRANSMITONLY_config(id);
			break;
		default:
			break;
	}
	HAL_SPI_Init(&hSPI[id]);
}

/**
 * @brief Fonction qui initialise les GPIOs pour une communication SPI en Full Duplex
 * @param SPI_id: SPI1_ID, SPI2_ID ou SPI3_ID
 */
void SPI_GPIO_FULLDUPLEX_config(SPI_ID_e SPI_id){
	switch (SPI_id) {
		case SPI1_ID:
			/* SPI1 clock enable */
			__HAL_RCC_SPI1_CLK_ENABLE();

			__HAL_RCC_GPIOA_CLK_ENABLE();
			/*SPI1 GPIO Configuration
			PA5     ------> SPI1_SCK
			PA6     ------> SPI1_MISO
			PA7     ------> SPI1_MOSI
			*/
			BSP_GPIO_pin_config(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI1);
			break;
		case SPI2_ID:
		    /* SPI2 clock enable */
		    __HAL_RCC_SPI2_CLK_ENABLE();

		    __HAL_RCC_GPIOF_CLK_ENABLE();
		    __HAL_RCC_GPIOA_CLK_ENABLE();
		    /*SPI2 GPIO Configuration
		    PF1-OSC_OUT ------> SPI2_SCK /!\ PF1 n'est accessible que si SB11 et pas SB10 (petits pads à souder/relier sur la carte stm32g431)
		    PA10     	------> SPI2_MISO
		    PA11     	------> SPI2_MOSI
		    */
		    BSP_GPIO_pin_config(GPIOF, GPIO_PIN_1, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI2);
		    BSP_GPIO_pin_config(GPIOA, GPIO_PIN_10|GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI2);
		    break;
		case SPI3_ID:
			/* SPI3 clock enable */
			__HAL_RCC_SPI3_CLK_ENABLE();

			__HAL_RCC_GPIOB_CLK_ENABLE();
			/*SPI3 GPIO Configuration
			PB3     ------> SPI3_SCK
			PB4     ------> SPI3_MISO
			PB5     ------> SPI3_MOSI
			*/
			BSP_GPIO_pin_config(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
			break;
		default:
			break;
	}
}

/**
 * @brief Fonction qui initialise les GPIOs pour une communication SPI en Half Duplex ou Transmit Only
 * @param SPI_id: SPI1_ID, SPI2_ID ou SPI3_ID
 */
void SPI_GPIO_HALFDUPLEX_or_TRANSMITONLY_config(SPI_ID_e SPI_id){
	switch (SPI_id) {
		case SPI1_ID:
			/* SPI1 clock enable */
			__HAL_RCC_SPI1_CLK_ENABLE();

			__HAL_RCC_GPIOA_CLK_ENABLE();
			/*SPI1 GPIO Configuration
			PA5     ------> SPI1_SCK
			PA6     ------> SPI1_MISO
			PA7     ------> SPI1_MOSI
			*/
			if (hSPI[SPI_id].Init.Mode == SPI_MODE_MASTER)
				BSP_GPIO_pin_config(GPIOA, GPIO_PIN_5|GPIO_PIN_7, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI1);
			else
				BSP_GPIO_pin_config(GPIOA, GPIO_PIN_5|GPIO_PIN_6, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI1);
			break;
		case SPI2_ID:
			/* SPI2 clock enable */
			__HAL_RCC_SPI2_CLK_ENABLE();

			__HAL_RCC_GPIOF_CLK_ENABLE();
			__HAL_RCC_GPIOA_CLK_ENABLE();
			/*SPI2 GPIO Configuration
			PF1-OSC_OUT ------> SPI2_SCK /!\ PF1 n'est accessible que si SB11 et pas SB10 (petits pads à souder/relier sur la carte stm32g431)
			PA10     	------> SPI2_MISO
			PA11     	------> SPI2_MOSI
			*/
		    BSP_GPIO_pin_config(GPIOF, GPIO_PIN_1, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI2);
			if (hSPI[SPI_id].Init.Mode == SPI_MODE_MASTER)
			    BSP_GPIO_pin_config(GPIOA, GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI2);
			else
			    BSP_GPIO_pin_config(GPIOA, GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI2);
			break;
		case SPI3_ID:
			/* SPI3 clock enable */
			__HAL_RCC_SPI3_CLK_ENABLE();

			__HAL_RCC_GPIOB_CLK_ENABLE();
			/*SPI3 GPIO Configuration
			PB3     ------> SPI3_SCK
			PB4     ------> SPI3_MISO
			PB5     ------> SPI3_MOSI
			*/
			if (hSPI[SPI_id].Init.Mode == SPI_MODE_MASTER)
				BSP_GPIO_pin_config(GPIOB, GPIO_PIN_3|GPIO_PIN_5, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
			else
				BSP_GPIO_pin_config(GPIOB, GPIO_PIN_3|GPIO_PIN_4, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
			break;
		default:
			break;
	}
}

/**
 * @brief Fonction qui initialise les GPIOs pour une communication SPI en mode Receive Only (tout est dans le nom)
 * @param SPI_id: SPI1_ID, SPI2_ID ou SPI3_ID
 */
void SPI_GPIO_RECEIVEONLY_config(SPI_ID_e SPI_id){
	switch (SPI_id) {
		case SPI1_ID:
			/* SPI1 clock enable */
			__HAL_RCC_SPI1_CLK_ENABLE();

			__HAL_RCC_GPIOA_CLK_ENABLE();
			/*SPI1 GPIO Configuration
			PA5     ------> SPI1_SCK
			PA6     ------> SPI1_MISO
			PA7     ------> SPI1_MOSI
			*/
			if (hSPI[SPI_id].Init.Mode == SPI_MODE_MASTER)
				BSP_GPIO_pin_config(GPIOA, GPIO_PIN_5|GPIO_PIN_6, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI1);
			else
				BSP_GPIO_pin_config(GPIOA, GPIO_PIN_5|GPIO_PIN_7, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI1);
			break;
		case SPI2_ID:
			/* SPI2 clock enable */
			__HAL_RCC_SPI2_CLK_ENABLE();

			__HAL_RCC_GPIOF_CLK_ENABLE();
			__HAL_RCC_GPIOA_CLK_ENABLE();
			/*SPI2 GPIO Configuration
			PF1-OSC_OUT ------> SPI2_SCK /!\ PF1 n'est accessible que si SB11 et pas SB10 (petits pads à souder/relier sur la carte stm32g431)
			PA10     	------> SPI2_MISO
			PA11     	------> SPI2_MOSI
			*/
			BSP_GPIO_pin_config(GPIOF, GPIO_PIN_1, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI2);
			if (hSPI[SPI_id].Init.Mode == SPI_MODE_MASTER)
				BSP_GPIO_pin_config(GPIOA, GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI2);
			else
				BSP_GPIO_pin_config(GPIOA, GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF5_SPI2);
			break;
		case SPI3_ID:
			/* SPI3 clock enable */
			__HAL_RCC_SPI3_CLK_ENABLE();

			__HAL_RCC_GPIOB_CLK_ENABLE();
			/*SPI3 GPIO Configuration
			PB3     ------> SPI3_SCK
			PB4     ------> SPI3_MISO
			PB5     ------> SPI3_MOSI
			*/
			if (hSPI[SPI_id].Init.Mode == SPI_MODE_MASTER)
				BSP_GPIO_pin_config(GPIOB, GPIO_PIN_3|GPIO_PIN_4, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
			else
				BSP_GPIO_pin_config(GPIOB, GPIO_PIN_3|GPIO_PIN_5, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
			break;
		default:
			break;
	}
}

/**
 * @brief Cette fonction sert à recevoir une donnée sur l'un des bus SPI.
 * @param SPIx: est le SPI à lire.
 * @return Cette fonction retourne la donnée lue sur le SPI choisi
 */
uint8_t BSP_SPI_ReadNoRegister(SPI_TypeDef* SPIx)
{
	uint8_t data;
	assert(SPIx == SPI1 || SPIx == SPI2 || SPIx == SPI3);
	SPI_ID_e id = ((SPIx == SPI1)?SPI1_ID:(SPIx == SPI2)?SPI2_ID:SPI3_ID);
	data = 0x00;
	HAL_SPI_Receive(&hSPI[id],&data,1,100);
	return data;
}

/**
 * @brief Cette fonction sert à recevoir plusieurs données sur l'un des bus SPI.
 * @param SPIx: le SPI à lire.
 * @param *data: la variable qui va stocker les données lues.
 * @param count: le nombre de données à recevoir.
 */
void BSP_SPI_ReadMultiNoRegister(SPI_TypeDef* SPIx, uint8_t* data, uint16_t count)
{
	assert(SPIx == SPI1 || SPIx == SPI2 || SPIx == SPI3);
	SPI_ID_e id = ((SPIx == SPI1)?SPI1_ID:(SPIx == SPI2)?SPI2_ID:SPI3_ID);
	HAL_SPI_Receive(&hSPI[id],data,count,100);
}

/**
 * @brief Cette fonction sert à envoyer une donnée sur l'un des bus SPI.
 * @param SPIx: le SPI sur lequel envoyer la donnée.
 * @param data: la donnée à envoyer.
 */
void BSP_SPI_WriteNoRegister(SPI_TypeDef* SPIx, uint8_t data)
{
	assert(SPIx == SPI1 || SPIx == SPI2 || SPIx == SPI3);
	SPI_ID_e id = ((SPIx == SPI1)?SPI1_ID:(SPIx == SPI2)?SPI2_ID:SPI3_ID);
	HAL_SPI_Transmit(&hSPI[id],&data,1,100);
}

/**
 * @brief Cette fonction sert à envoyer plusieurs données sur l'un des bus SPI.
 * @param SPIx: le SPI sur lequel envoyer les données.
 * @param *data: la donnée à envoyer.
 * @param count: le nombre de données à envoyer.
 */
void BSP_SPI_WriteMultiNoRegister(SPI_TypeDef* SPIx, uint8_t* data, uint16_t count)
{
	assert(SPIx == SPI1 || SPIx == SPI2 || SPIx == SPI3);
	SPI_ID_e id = ((SPIx == SPI1)?SPI1_ID:(SPIx == SPI2)?SPI2_ID:SPI3_ID);
	HAL_SPI_Transmit(&hSPI[id],data,count,100);
}


uint8_t BSP_SPI_WriteRead(SPI_TypeDef* SPIx, uint8_t Value)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t readvalue = 0;
	assert(SPIx == SPI1 || SPIx == SPI2 || SPIx == SPI3);
	SPI_ID_e id = ((SPIx == SPI1)?SPI1_ID:(SPIx == SPI2)?SPI2_ID:SPI3_ID);
	status = HAL_SPI_TransmitReceive(&hSPI[id], (uint8_t*) &Value, (uint8_t*) &readvalue, 1, 100);

	/* Check the communication status */
	if(status != HAL_OK)
	{
	/* Execute user timeout callback */
	// SPIx_Error();
	}
	return readvalue;
}


/**
  * @brief  Write a byte on the SD.
  * @param  DataIn: byte to send.
  * @param  DataOut: byte to read
  * @param  DataLength: length of data
  */
void BSP_SPI_WriteReadBuffer(SPI_TypeDef* SPIx, const uint8_t *DataIn, uint8_t *DataOut, uint16_t DataLength)
{
  /* Send the byte */
	uint16_t i;
	for(i=0;i<DataLength;i++)
		DataOut[i] = (uint8_t)(BSP_SPI_WriteRead(SPIx, DataIn[i]));
}


/*
 * @brief Cette fonction sert à régler la taille d'une donnée
 * @param SPIx le SPI dont on veut régler la taille des donnée.
 * @param DataSize est la taille des donnée :
 * 						SPI_DATASIZE_8BIT  pour configurer le SPI en mode 8-bits
 * 						SPI_DATASIZE_16BIT pour configurer le SPI en mode 16-bits
 */
void BSP_SPI_SetDataSize(SPI_TypeDef* SPIx, uint32_t DataSize)
{
	SPI_ID_e id = ((SPIx == SPI2)?SPI2_ID:SPI1_ID);

	/* Disable SPI first */
	SPIx->CR1 &= ~SPI_CR1_SPE;

	/* Set proper value */
	SPIx->CR2 &= ~SPI_CR2_DS_Msk;
	hSPI[id].Init.DataSize = DataSize;
	SPIx->CR2 |= (DataSize & SPI_CR2_DS_Msk);

	/* Enable SPI back */
	SPIx->CR1 |= SPI_CR1_SPE;
}

/**
 * @author louisz (portage spi clubrobot)
 * @brief Permet d'envoyer une commande sur le bus SPI.
 * @param SPIx le SPI dont sur lequel on veut envoyer la commande.
 * @param NewState etat a envoyer comme commande au SPI
 */
void SPI_Cmd(SPI_TypeDef* SPIx, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    /* Enable the selected SPI peripheral */
	 SPIx->CR1 |= SPI_CR1_SPE;
  }
  else
  {
    /* Disable the selected SPI peripheral */
	 SPIx->CR1 &= (uint16_t)~((uint16_t)SPI_CR1_SPE);
  }
}


/**
 * @author louisz (portage spi clubrobot)
 * @brief Permet de régler le baudRate d'un SPI.
 * @param SPIx le SPI dont on veut modifier le baudRate.
 * @param BaudRate choisi, voir SPI_BAUDRATEPRESCALER_x où x vaut 2, 4, 8, 16, 32, 64, 128, 256
 * @pre SPI_Init(SPIx) must be called before
 */
void BSP_SPI_setBaudRate(SPI_TypeDef* SPIx, uint16_t SPI_BaudRatePrescaler)
{
	assert(SPIx == SPI1 || SPIx == SPI2 || SPIx == SPI3);
	SPI_ID_e id = ((SPIx == SPI1)?SPI1_ID:(SPIx == SPI2)?SPI2_ID:SPI3_ID);
	SPI_Cmd(SPIx, DISABLE);
	hSPI[id].Init.BaudRatePrescaler = SPI_BaudRatePrescaler;
	HAL_SPI_Init(&hSPI[id]);
	SPI_Cmd(SPIx, ENABLE);
}

uint32_t BSP_SPI_getBaudrate(SPI_TypeDef* SPIx)
{
	assert(SPIx == SPI1 || SPIx == SPI2 || SPIx == SPI3);
	SPI_ID_e id = ((SPIx == SPI1)?SPI1_ID:(SPIx == SPI2)?SPI2_ID:SPI3_ID);
	return hSPI[id].Init.BaudRatePrescaler;
}



