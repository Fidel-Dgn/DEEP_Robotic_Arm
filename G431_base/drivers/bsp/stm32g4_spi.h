/**
 *******************************************************************************
 * @file	stm32g4_spi.h
 * @author	vchav
 * @date	May 14, 2024
 * @brief	Module SPI pour cible stm32g4
 * 			Adaptation du module stm32f1_spi.c de Nirgal
 *******************************************************************************
 */

#ifndef BSP_STM32G4_SPI_H_
#define BSP_STM32G4_SPI_H_

#include "stm32g431xx.h"


/* Public enumerations declarations ------------------------------------------*/
typedef enum{
	FULL_DUPLEX,
	HALF_DUPLEX,
	RECEIVE_ONLY,
	TRANSMIT_ONLY,
}SPI_Mode_e;

typedef enum{
	MASTER,
	SLAVE
}SPI_Rank_e;

typedef enum {
	TM_SPI_DataSize_8b, /*!< SPI in 8-bits mode */
	TM_SPI_DataSize_16b /*!< SPI in 16-bits mode */
} TM_SPI_DataSize_t;

/* Public functions declarations ---------------------------------------------*/
void BSP_SPI_Init(SPI_TypeDef* SPIx, SPI_Mode_e SPI_Mode, SPI_Rank_e SPI_Rank, uint16_t SPI_BAUDRATEPRESCALER_x);

uint8_t BSP_SPI_ReadNoRegister(SPI_TypeDef* SPIx);

void BSP_SPI_ReadMultiNoRegister(SPI_TypeDef* SPIx, uint8_t* data, uint16_t count);

void BSP_SPI_WriteNoRegister(SPI_TypeDef* SPIx, uint8_t data);

void BSP_SPI_WriteMultiNoRegister(SPI_TypeDef* SPIx, uint8_t* data, uint16_t count);

uint8_t BSP_SPI_WriteRead(SPI_TypeDef* SPIx, uint8_t Value);

void BSP_SPI_WriteReadBuffer(SPI_TypeDef* SPIx, const uint8_t *DataIn, uint8_t *DataOut, uint16_t DataLength);

void BSP_SPI_setBaudRate(SPI_TypeDef* SPIx, uint16_t SPI_BaudRatePrescaler);

uint32_t BSP_SPI_getBaudrate(SPI_TypeDef* SPIx);

void BSP_SPI_SetDataSize(SPI_TypeDef* SPIx, uint32_t DataSize);


#endif /* BSP_STM32G4_SPI_H_ */
