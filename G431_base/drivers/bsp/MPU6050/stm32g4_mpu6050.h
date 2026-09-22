/**
 *******************************************************************************
 * @file 	stm32g4_mpu6050.h
 * @author 	Tilen Majerle (2014) (cf license ci dessous)
 * @author  Samuel Poiraud (2016) -> portage STM32F1 et modifications pour les activités pédagogiques à l'ESEO.
 * @author	vchav (2024) -> portage sur STM32G4
 * @date 	May 3, 2024
 * @brief	Module pour utiliser le capteur MPU6050.
 *******************************************************************************
 * @verbatim--------------------------------------------------------------
 * | Copyright (C) Tilen Majerle, 2014
 * |
 * | This program is free software: you can redistribute it and/or modify
 * | it under the terms of the GNU General Public License as published by
 * | the Free Software Foundation, either version 3 of the License, or
 * | any later version.
 * |
 * | This program is distributed in the hope that it will be useful,
 * | but WITHOUT ANY WARRANTY; without even the implied warranty of
 * | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * | GNU General Public License for more details.
 * |
 * | You should have received a copy of the GNU General Public License
 * | along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * |-----------------------------------------------------------@endverbatim
 */

#ifndef BSP_MPU6050_STM32G4_MPU6050_H_
#define BSP_MPU6050_STM32G4_MPU6050_H_
#include "config.h"
#if USE_MPU6050

/* Exported macros -----------------------------------------------------------*/
/* I2C par défaut utilisé */
#ifndef MPU6050_I2C
	#define	MPU6050_I2C					I2C1
#endif

/* Horloge I2C par défaut */
#ifndef MPU6050_I2C_CLOCK
#define MPU6050_I2C_CLOCK			400000
#endif

/* Adresse I2C par défaut */
#define MPU6050_I2C_ADDR			0xD0

/* Valeur du registre Who I am en fonction du capteur*/
#define MPU6050_I_AM				0x68
#define MPU9250_I_AM				0x71
#define MPU6060_I_AM_STRANGE_MODEL	0x72
#define MPU9255_I_AM				0x73

/* Registres du MPU6050 */
#define MPU6050_AUX_VDDIO			0x01
#define MPU6050_SMPLRT_DIV			0x19
#define MPU6050_CONFIG				0x1A
#define MPU6050_GYRO_CONFIG			0x1B
#define MPU6050_ACCEL_CONFIG		0x1C
#define MPU6050_MOTION_THRESH		0x1F
#define MPU6050_INT_PIN_CFG			0x37
#define MPU6050_INT_ENABLE			0x38
#define MPU6050_INT_STATUS			0x3A
#define MPU6050_ACCEL_XOUT_H		0x3B
#define MPU6050_ACCEL_XOUT_L		0x3C
#define MPU6050_ACCEL_YOUT_H		0x3D
#define MPU6050_ACCEL_YOUT_L		0x3E
#define MPU6050_ACCEL_ZOUT_H		0x3F
#define MPU6050_ACCEL_ZOUT_L		0x40
#define MPU6050_TEMP_OUT_H			0x41
#define MPU6050_TEMP_OUT_L			0x42
#define MPU6050_GYRO_XOUT_H			0x43
#define MPU6050_GYRO_XOUT_L			0x44
#define MPU6050_GYRO_YOUT_H			0x45
#define MPU6050_GYRO_YOUT_L			0x46
#define MPU6050_GYRO_ZOUT_H			0x47
#define MPU6050_GYRO_ZOUT_L			0x48
#define MPU6050_MOT_DETECT_STATUS	0x61
#define MPU6050_SIGNAL_PATH_RESET	0x68
#define MPU6050_MOT_DETECT_CTRL		0x69
#define MPU6050_USER_CTRL			0x6A
#define MPU6050_PWR_MGMT_1			0x6B
#define MPU6050_PWR_MGMT_2			0x6C
#define MPU6050_FIFO_COUNTH			0x72
#define MPU6050_FIFO_COUNTL			0x73
#define MPU6050_FIFO_R_W			0x74
#define MPU6050_WHO_AM_I			0x75

/* Sensibilités du gyroscope en °/s */
#define MPU6050_GYRO_SENS_250		((float) 131)
#define MPU6050_GYRO_SENS_500		((float) 65.5)
#define MPU6050_GYRO_SENS_1000		((float) 32.8)
#define MPU6050_GYRO_SENS_2000		((float) 16.4)

/* Sensibilités de l'accéléromètre en g */
#define MPU6050_ACCE_SENS_2			((float) 16384)
#define MPU6050_ACCE_SENS_4			((float) 8192)
#define MPU6050_ACCE_SENS_8			((float) 4096)
#define MPU6050_ACCE_SENS_16		((float) 2048)



/* Exported types ------------------------------------------------------------*/

/**
 * @brief  Le MPU6050 peut avoir 2 adresses esclaves différentes, selon l'état de sa broche AD0.
 *         Cette fonctionnalité vous permet d'utiliser 2 capteurs différents avec cette bibliothèque en même temps.
 */
typedef enum {
	MPU6050_Device_0 = 0,   /*!< La broche AD0 est configurée sur état bas */
	MPU6050_Device_1 = 0x02 /*!< La broche AD0 est configurée sur état haut */
} MPU6050_Device_t;

/* Énumération des résultats du MPU6050 */
typedef enum {
	MPU6050_Result_Ok = 0x00,          /*!< Tout est OK */
	MPU6050_Result_DeviceNotConnected, /*!< Aucun périphérique avec une adresse esclave valide */
	MPU6050_Result_DeviceInvalid       /*!< Périphérique connecté avec une adresse invalide */
} MPU6050_Result_t;

/* Paramètres pour la plage de l'accéléromètre */
typedef enum {
	MPU6050_Accelerometer_2G = 0x00, /*!< Plage de +- 2G */
	MPU6050_Accelerometer_4G = 0x01, /*!< Plage de +- 4G */
	MPU6050_Accelerometer_8G = 0x02, /*!< Plage de +- 8G */
	MPU6050_Accelerometer_16G = 0x03 /*!< Plage de +- 16G */
} MPU6050_Accelerometer_t;

/* Paramètres pour la plage du gyroscope */
typedef enum {
	MPU6050_Gyroscope_250s = 0x00,  /*!< Plage de +- 250 degrés/s */
	MPU6050_Gyroscope_500s = 0x01,  /*!< Plage de +- 500 degrés/s */
	MPU6050_Gyroscope_1000s = 0x02, /*!< Plage de +- 1000 degrés/s */
	MPU6050_Gyroscope_2000s = 0x03  /*!< Plage de +- 2000 degrés/s */
} MPU6050_Gyroscope_t;

/* Structure principale du MPU6050 */
typedef struct {
	/* Privé */
	uint8_t Address;         /*!< Adresse I2C du périphérique. Seulement pour un usage privé */
	float Gyro_Mult;         /*!< Correcteur du gyroscope pour convertir les données brutes en "degrés/s". Seulement pour un usage privé */
	float Acce_Mult;         /*!< Correcteur de l'accéléromètre pour convertir les données brutes en "g". Seulement pour un usage privé */
	/* Public */
	int16_t Accelerometer_X; /*!< Valeur de l'accéléromètre axe X */
	int16_t Accelerometer_Y; /*!< Valeur de l'accéléromètre axe Y */
	int16_t Accelerometer_Z; /*!< Valeur de l'accéléromètre axe Z */
	int16_t Gyroscope_X;     /*!< Valeur du gyroscope axe X */
	int16_t Gyroscope_Y;     /*!< Valeur du gyroscope axe Y */
	int16_t Gyroscope_Z;     /*!< Valeur du gyroscope axe Z */
	float Temperature;       /*!< Température en degrés */
} MPU6050_t;


/**
 * @defgroup MPU6050_Functions
 * @brief    Library Functions
 * @{
 */

/**
 * @brief  Classic use of the MPU6050. Might be taken as an exmaple of how to use it
 * @param  none
 * @retval none
 */
void MPU6050_demo(void);

/**
 * @brief  Initializes MPU6050 and I2C peripheral
 * @param  *DataStruct: Pointer to empty @ref MPU6050_t structure
 * @param   DeviceNumber: MPU6050 has one pin, AD0 which can be used to set address of device.
 *          This feature allows you to use 2 different sensors on the same board with same library.
 *          If you set AD0 pin to low, then this parameter should be MPU6050_Device_0,
 *          but if AD0 pin is high, then you should use MPU6050_Device_1
 *
 *          Parameter can be a value of @ref MPU6050_Device_t enumeration
 * @param  AccelerometerSensitivity: Set accelerometer sensitivity. This parameter can be a value of @ref MPU6050_Accelerometer_t enumeration
 * @param  GyroscopeSensitivity: Set gyroscope sensitivity. This parameter can be a value of @ref MPU6050_Gyroscope_t enumeration
 * @retval Status:
 *            - MPU6050_Result_t: Everything OK
 *            - Other member: in other cases
 */
MPU6050_Result_t MPU6050_Init(MPU6050_t* DataStruct, GPIO_TypeDef * GPIOx, uint16_t GPIO_PIN_x, MPU6050_Device_t DeviceNumber, MPU6050_Accelerometer_t AccelerometerSensitivity, MPU6050_Gyroscope_t GyroscopeSensitivity);
/**
 * @brief  Reads accelerometer data from sensor
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure to store data to
 * @retval Member of @ref MPU6050_Result_t:
 *            - MPU6050_Result_Ok: everything is OK
 *            - Other: in other cases
 */
MPU6050_Result_t MPU6050_ReadAccelerometer(MPU6050_t* DataStruct);

/**
 * @brief  Reads gyroscope data from sensor
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure to store data to
 * @retval Member of @ref MPU6050_Result_t:
 *            - MPU6050_Result_Ok: everything is OK
 *            - Other: in other cases
 */
MPU6050_Result_t MPU6050_ReadGyroscope(MPU6050_t* DataStruct);

/**
 * @brief  Reads temperature data from sensor
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure to store data to
 * @retval Member of @ref MPU6050_Result_t:
 *            - MPU6050_Result_Ok: everything is OK
 *            - Other: in other cases
 */
MPU6050_Result_t MPU6050_ReadTemperature(MPU6050_t* DataStruct);

/**
 * @brief  Reads accelerometer, gyroscope and temperature data from sensor
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure to store data to
 * @retval Member of @ref MPU6050_Result_t:
 *            - MPU6050_Result_Ok: everything is OK
 *            - Other: in other cases
 */
MPU6050_Result_t MPU6050_ReadAll(MPU6050_t* DataStruct);


#endif /* BSP_MPU6050_STM32G4_MPU6050_H_ */
#endif
