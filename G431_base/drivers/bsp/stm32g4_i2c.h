/**
 *******************************************************************************
 * @file 	stm32g4_i2c.h
 * @author 	vchav
 * @date 	Apr 29, 2024
 * @brief	Module i2c pour cible stm32g4
 *******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BSP_STM32G4_I2C_H_
#define BSP_STM32G4_I2C_H_

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#if USE_I2C

#include "stm32g4_utils.h"
#include "stm32g4xx_hal_def.h"

/* Public enumerations declarations ------------------------------------------*/
typedef enum
{
	I2C1_NORMAL,
	I2C2_NORMAL,
	I2C3_NORMAL,
	I2C_NB,
}I2C_id_e;

typedef enum{
	STANDARD_MODE, 	// Speed frequency: 100KHz
	FAST_MODE, 		// Speed frequency: 400KHz
	SUPERFAST_MODE 	// Speed frequency: 1000KHz
}I2C_speed_mode_e;


/* Public functions declarations ---------------------------------------------*/
HAL_StatusTypeDef BSP_I2C_Init(I2C_TypeDef* I2Cx, I2C_speed_mode_e speed_mode, bool analog_filter);

HAL_StatusTypeDef BSP_I2C_Read(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t * received_data);

HAL_StatusTypeDef BSP_I2C_ReadMulti(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t *data, uint16_t count);

HAL_StatusTypeDef BSP_I2C_ReadNoRegister(I2C_TypeDef* I2Cx, uint8_t address, uint8_t * data);

HAL_StatusTypeDef BSP_I2C_ReadMultiNoRegister(I2C_TypeDef* I2Cx, uint8_t address, uint8_t* data, uint16_t count);

HAL_StatusTypeDef BSP_I2C_Write(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data);

HAL_StatusTypeDef BSP_I2C_WriteMulti(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t* data, uint16_t count);

HAL_StatusTypeDef BSP_I2C_WriteNoRegister(I2C_TypeDef* I2Cx, uint8_t address, uint8_t data);

HAL_StatusTypeDef BSP_I2C_WriteMultiNoRegister(I2C_TypeDef* I2Cx, uint8_t address, uint8_t* data, uint16_t count);

bool BSP_I2C_IsDeviceConnected(I2C_TypeDef* I2Cx, uint8_t address);

I2C_HandleTypeDef * BSP_I2C_get_handle(I2C_TypeDef* I2Cx);


#endif
#endif /* BSP_STM32G4_I2C_H_ */
