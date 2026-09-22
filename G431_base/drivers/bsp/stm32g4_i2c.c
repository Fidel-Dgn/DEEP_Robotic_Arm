/**
 *******************************************************************************
 * @file 	stm32g4_i2c.c
 * @author 	vchav
 * @date 	Apr 29, 2024
 * @brief	Module i2c pour cible stm32g4
 *******************************************************************************
 */

#include "config.h"
#if USE_I2C

/* Includes ------------------------------------------------------------------*/
#include "stm32g4_i2c.h"
#include "stm32g4_gpio.h"
#include "stm32g4_sys.h"
#include <assert.h>

/* Private variables ---------------------------------------------------------*/
static I2C_HandleTypeDef  hi2c[I2C_NB];



/**
 * @brief Initialise l'I2C
 * @param *I2Cx: I2C utilisé
 * @param speed_mode: Mode de vitesse qui influencera la fréquence de communication
 * @param analog_filter: = true si vous souhaitez activer le filtrage analogique (cela augmentera le temps de traitement mais améliorera la qualité de communication)
 * @return HAL_StatusTypeDef: Retourne HAL_OK si l'initialisation est réussie, sinon une erreur est signalée
 */
HAL_StatusTypeDef BSP_I2C_Init(I2C_TypeDef* I2Cx, I2C_speed_mode_e speed_mode, bool analog_filter){

	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx== I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));

	uint32_t timing;
	switch(speed_mode)
	{
		case FAST_MODE:
			if(analog_filter){
				timing=0x10802D9B;
			}else{
				timing=0x108031A0;
			}
			break;
		case SUPERFAST_MODE:
			if(analog_filter){
				timing=0x0080272;
			}else{
				timing=0x00802C78;
			}
			break;
		case STANDARD_MODE:
			//no break;
		default:
			if(analog_filter){
				timing=0x30A0A7FB;
			}else{
				timing=0x30A0A9FB;
			}
			break;
	}

	hi2c[id].Instance = I2Cx;
	hi2c[id].Init.Timing = timing;
	hi2c[id].Init.OwnAddress1 = 0;
	hi2c[id].Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c[id].Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c[id].Init.OwnAddress2 = 0;
	hi2c[id].Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c[id].Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c[id].Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

	if (HAL_I2C_Init(&hi2c[id]) != HAL_OK)
	{
		Error_Handler();
	}

	/* Configure Analogue filter*/
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c[id], (analog_filter)?(I2C_ANALOGFILTER_ENABLE):(I2C_ANALOGFILTER_DISABLE)) != HAL_OK)
	{
		Error_Handler();
	}

	/* Configure Digital filter*/
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c[id], 0) != HAL_OK)
	{
		Error_Handler();
	}
	return HAL_OK;
}


/**
 * @brief Initialise les ressources de bas niveau de l'I2C (configuration des pins, horloge de l'interface)
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* Initializes the peripherals clocks */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /*I2C1 GPIO Configuration
    PA15     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    BSP_GPIO_pin_config(GPIOA, GPIO_PIN_15, GPIO_MODE_AF_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF4_I2C1);
    BSP_GPIO_pin_config(GPIOB, GPIO_PIN_7, GPIO_MODE_AF_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF4_I2C1);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  }
  else if(i2cHandle->Instance==I2C2)
  {

  /* Initializes the peripherals clocks */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
    PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /*I2C2 GPIO Configuration
    PF0-OSC_IN     ------> I2C2_SDA
    PA9     ------> I2C2_SCL
    */
    BSP_GPIO_pin_config(GPIOF, GPIO_PIN_0, GPIO_MODE_AF_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF4_I2C2);
    BSP_GPIO_pin_config(GPIOA, GPIO_PIN_9, GPIO_MODE_AF_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF4_I2C2);

    /* I2C2 clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();
  }
  else if(i2cHandle->Instance==I2C3)
  {
  /* Initializes the peripherals clocks */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
    PeriphClkInit.I2c3ClockSelection = RCC_I2C3CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /*I2C3 GPIO Configuration
    PA8     ------> I2C3_SCL
    PB5     ------> I2C3_SDA
    */
    BSP_GPIO_pin_config(GPIOA, GPIO_PIN_8, GPIO_MODE_AF_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF2_I2C3);
    BSP_GPIO_pin_config(GPIOB, GPIO_PIN_5, GPIO_MODE_AF_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF8_I2C3);

    /* I2C3 clock enable */
    __HAL_RCC_I2C3_CLK_ENABLE();
  }
}


/**
 * @brief Deinitialise les ressources de bas niveau de l'I2C (configuration des pins, horloge de l'interface)
 */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /*I2C1 GPIO Configuration
    PA15     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

  }
  else if(i2cHandle->Instance==I2C2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();

    /*I2C2 GPIO Configuration
    PF0-OSC_IN     ------> I2C2_SDA
    PA9     ------> I2C2_SCL
    */
    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_0);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);

  }
  else if(i2cHandle->Instance==I2C3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C3_CLK_DISABLE();

    /*I2C3 GPIO Configuration
    PA8     ------> I2C3_SCL
    PB5     ------> I2C3_SDA
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5);
  }
}


/**
 * @brief  Lit un octet de l'esclave
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @param  reg: Registre à partir duquel lire
 * @return La donnée lue
 */
HAL_StatusTypeDef I2C_Read16(I2C_TypeDef* I2Cx, uint16_t address, uint8_t reg, uint8_t * received_data)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Mem_Read(&hi2c[id],address,reg,I2C_MEMADD_SIZE_16BIT,received_data,1,I2C_TIMEOUT);
}


/**
 * @brief  Lit un octet de l'esclave
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @param  reg: Registre à partir duquel lire
 * @return La donnée lue
 */
HAL_StatusTypeDef BSP_I2C_Read(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t * received_data)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Mem_Read(&hi2c[id],address,reg,I2C_MEMADD_SIZE_8BIT,received_data,1,I2C_TIMEOUT);
}


/**
 * @brief  Lit plusieurs octets de l'esclave
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @param  reg: registre à lire
 * @param  *data: pointeur vers le tableau de données pour stocker les données de l'esclave
 * @param  count: nombre d'octets à lire
 */
HAL_StatusTypeDef BSP_I2C_ReadMulti(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t* data, uint16_t count)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Mem_Read(&hi2c[id],address,reg,I2C_MEMADD_SIZE_8BIT,data,count,I2C_TIMEOUT*20);
}


/**
 * @brief  Lit un octet de l'esclave sans spécifier l'adresse du registre
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @return La donnée lue
 */
HAL_StatusTypeDef BSP_I2C_ReadNoRegister(I2C_TypeDef* I2Cx, uint8_t address, uint8_t * data)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Master_Receive(&hi2c[id],address,data,1,I2C_TIMEOUT);
}


/**
 * @brief  Lit plusieurs octets de l'esclave sans spécifier le registre de départ
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @param  *data: pointeur vers le tableau de données pour stocker les données de l'esclave
 * @param  count: nombre d'octets à lire
 */
HAL_StatusTypeDef BSP_I2C_ReadMultiNoRegister(I2C_TypeDef* I2Cx, uint8_t address, uint8_t* data, uint16_t count)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Master_Receive(&hi2c[id],address,data,count,I2C_TIMEOUT*20);
}



/**
 * @brief  Écrit un octet à l'esclave
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @param  reg: registre où écrire
 * @param  data: donnée à écrire
 */
HAL_StatusTypeDef BSP_I2C_Write(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Mem_Write(&hi2c[id],address,reg,I2C_MEMADD_SIZE_8BIT,&data,1,I2C_TIMEOUT);
}


/**
 * @brief  Écrit plusieurs octets à l'esclave
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @param  reg: registre où écrire
 * @param  *data: pointeur vers le tableau de données à écrire sur l'esclave
 * @param  count: nombre d'octets à écrire
 */
HAL_StatusTypeDef BSP_I2C_WriteMulti(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t* data, uint16_t count)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Mem_Write(&hi2c[id],address,reg,I2C_MEMADD_SIZE_8BIT,data,count,I2C_TIMEOUT*20);
}


/**
 * @brief  Écrit un octet à l'esclave sans spécifier l'adresse du registre.
 *
 *         Utile si vous avez un périphérique I2C à lire de cette manière :
 *            - I2C START
 *            - ENVOYER L'ADRESSE DU PÉRIPHÉRIQUE
 *            - ENVOYER L'OCTET DE DONNÉES
 *            - I2C STOP
 *
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @param  data: octet de données qui sera envoyé au périphérique
 */
HAL_StatusTypeDef BSP_I2C_WriteNoRegister(I2C_TypeDef* I2Cx, uint8_t address, uint8_t data)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Master_Transmit(&hi2c[id],address,&data,1,I2C_TIMEOUT);
}


/**
 * @brief  Écrit plusieurs octets à l'esclave sans définir le registre à partir duquel commencer l'écriture.
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse 7 bits de l'esclave, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @param  *data: pointeur vers le tableau de données à écrire sur l'esclave
 * @param  count: nombre d'octets que vous souhaitez écrire
 */
HAL_StatusTypeDef BSP_I2C_WriteMultiNoRegister(I2C_TypeDef* I2Cx, uint8_t address, uint8_t* data, uint16_t count)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return HAL_I2C_Master_Transmit(&hi2c[id],address,data,count,I2C_TIMEOUT*20);
}


/**
 * @brief  Vérifie si un appareil est connecté au bus I2C
 * @param  *I2Cx: I2C utilisé
 * @param  address: Adresse esclave sur 7 bits, alignée à gauche, les bits 7:1 sont utilisés, le bit LSB n'est pas utilisé
 * @return État de l'appareil :
 *            - false: L'appareil n'est pas connecté
 *            - true: L'appareil est connecté
 */
bool BSP_I2C_IsDeviceConnected(I2C_TypeDef* I2Cx, uint8_t address)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return (HAL_I2C_IsDeviceReady(&hi2c[id],address,2,I2C_TIMEOUT) == HAL_OK)?true:false;
}


I2C_HandleTypeDef * BSP_I2C_get_handle(I2C_TypeDef* I2Cx)
{
	assert(I2Cx == I2C1 || I2Cx == I2C2 || I2Cx == I2C3);
	I2C_id_e id = ((I2Cx == I2C1)?I2C1_NORMAL:((I2Cx == I2C2)?I2C2_NORMAL:I2C3_NORMAL));
	return &hi2c[id];
}

#endif
