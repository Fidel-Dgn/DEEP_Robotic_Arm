/**
 *******************************************************************************
 * @file 	stm32g4_mpu6050.c
 * @author 	Tilen Majerle (2014) (cf license ci dessous)
 * @author  Samuel Poiraud (2016) -> portage STM32F1 et modifications pour les activités pédagogiques à l'ESEO.
 * @author	vchav (2024) -> portage sur STM32G4
 * @date 	May 6, 2024
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

/*
 * Driver fourni pour l'accéléromètre-gyroscope MPU6050.
 * Brochage proposé par défaut :
 *
 *  Par défaut, les broches utilisées sont celles de l'I2C1 :
 *  		- SCL : PA15
 *  		- SDA : PB7
 *  		- Vcc : PA0 par exemple (cf MPU6050_Init) !!! (ce qui permet au pilote de couper l'alimentation et provoquer un reset).
 *  		- GND : GND de la carte
 *
 * Le basculement sur l'I2C2 ou l'I2C3 est possible en redéfinissant MPU6050_I2C = I2C2 ou I2C3
 * I2C2: PA9 pour SCL et PF0 pour SDA   ----+
 * I2C3: PA8 pour SCL et PB5 pour SDA		|
 * 											|
 *											V
 * +------------------------------------------------------------------------------------------------------------------------------
 * | Pour l'I2C2:
 * | /!\ Le PF0 n'est disponible que si SB8 est soudé et pas SB13 (petits pads sur la carte stm32g431) /!\
 * | Pour mieux comprendre la manipulation à faire pour débloquer PF0, je vous renvoie à la documentation page 4: https://www.st.com/content/ccc/resource/technical/layouts_and_diagrams/schematic_pack/group1/f5/e0/9b/fb/40/5f/43/b3/MB1430-G431KBT6-A02_Schematic_Internal/files/MB1430-G431KBT6-A02_Schematic_Internal.pdf/jcr:content/translations/en.MB1430-G431KBT6-A02_Schematic_Internal.pdf
 * +------------------------------------------------------------------------------------------------------------------------------
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#if USE_MPU6050
#include "stm32g4_i2c.h"
#include "stm32g4_mpu6050.h"
#include "stm32g4_gpio.h"
#include <stdio.h>

/**
 * @brief	Initialise le module MPU6050 en activant son alimentation, puis en configurant les registres internes du MPU6050.
 * @param	GPIOx et GPIO_PIN_x : indiquent la broche où l'on a relié l'alimentation Vcc du MPU6050.
 * 			Indiquer NULL dans GPIOx s'il est alimenté en direct.
 * 			Cette possibilité d'alimentation par la broche permet le reset du module par le microcontrôleur.
 * @param	DataStruct : fournir le pointeur vers une structure qui sera à conserver pour les autres appels des fonctions de ce module logiciel.
 * @param 	DeviceNumber : 					voir MPU6050_Device_t
 * @param	AccelerometerSensitivity : 		voir MPU6050_Accelerometer_t
 * @param	GyroscopeSensitivity :			voir MPU6050_Gyroscope_t
 */
MPU6050_Result_t MPU6050_Init(MPU6050_t* DataStruct, GPIO_TypeDef * GPIOx, uint16_t GPIO_PIN_x, MPU6050_Device_t DeviceNumber, MPU6050_Accelerometer_t AccelerometerSensitivity, MPU6050_Gyroscope_t GyroscopeSensitivity)
{
	uint8_t temp;

	if(GPIOx != NULL)
	{
		BSP_GPIO_pin_config(GPIOx, GPIO_PIN_x,GPIO_MODE_OUTPUT_PP,GPIO_PULLUP,GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
		HAL_GPIO_WritePin(GPIOx,GPIO_PIN_x,GPIO_PIN_RESET);
		HAL_Delay(20);
		HAL_GPIO_WritePin(GPIOx,GPIO_PIN_x,GPIO_PIN_SET);
	}
	HAL_Delay(20);

	/* Formate l'addresse de l'I2C */
	DataStruct->Address = MPU6050_I2C_ADDR | (uint8_t)DeviceNumber;

	/* Initialise l'I2C */
	BSP_I2C_Init(MPU6050_I2C, STANDARD_MODE, true);

	/* On vérifie que le capteur est bien connecté */
	if (!BSP_I2C_IsDeviceConnected(MPU6050_I2C, DataStruct->Address)) {
		/* Return error */
		return MPU6050_Result_DeviceNotConnected;
	}

	/* Check le "who I am" */
	uint8_t i_am;
	BSP_I2C_Read(MPU6050_I2C, DataStruct->Address, MPU6050_WHO_AM_I, &i_am);
	if (i_am != MPU6050_I_AM && i_am != MPU9250_I_AM && i_am != MPU9255_I_AM && i_am != MPU6060_I_AM_STRANGE_MODEL) {
		/* Return error */
		return MPU6050_Result_DeviceInvalid;
	}

	/* On réveil le MPU6050 */
	BSP_I2C_Write(MPU6050_I2C, DataStruct->Address, MPU6050_PWR_MGMT_1, 0x00);

	/* On config l'accéléromètre */
	BSP_I2C_Read(MPU6050_I2C, DataStruct->Address, MPU6050_ACCEL_CONFIG, &temp);
	temp = (temp & 0xE7) | (uint8_t)AccelerometerSensitivity << 3;
	BSP_I2C_Write(MPU6050_I2C, DataStruct->Address, MPU6050_ACCEL_CONFIG, temp);

	/* On config le gyroscope */
	BSP_I2C_Read(MPU6050_I2C, DataStruct->Address, MPU6050_GYRO_CONFIG, &temp);
	temp = (temp & 0xE7) | (uint8_t)GyroscopeSensitivity << 3;
	BSP_I2C_Write(MPU6050_I2C, DataStruct->Address, MPU6050_GYRO_CONFIG, temp);

	/* On définie les sensiblités pour multiplier les données du gyroscope et de l'accéléromètre */
	switch (AccelerometerSensitivity) {
		case MPU6050_Accelerometer_2G:
			DataStruct->Acce_Mult = (float)1 / MPU6050_ACCE_SENS_2;
			break;
		case MPU6050_Accelerometer_4G:
			DataStruct->Acce_Mult = (float)1 / MPU6050_ACCE_SENS_4;
			break;
		case MPU6050_Accelerometer_8G:
			DataStruct->Acce_Mult = (float)1 / MPU6050_ACCE_SENS_8;
			break;
		case MPU6050_Accelerometer_16G:
			DataStruct->Acce_Mult = (float)1 / MPU6050_ACCE_SENS_16;
			//no break
		default:
			break;
	}

	switch (GyroscopeSensitivity) {
		case MPU6050_Gyroscope_250s:
			DataStruct->Gyro_Mult = (float)1 / MPU6050_GYRO_SENS_250;
			break;
		case MPU6050_Gyroscope_500s:
			DataStruct->Gyro_Mult = (float)1 / MPU6050_GYRO_SENS_500;
			break;
		case MPU6050_Gyroscope_1000s:
			DataStruct->Gyro_Mult = (float)1 / MPU6050_GYRO_SENS_1000;
			break;
		case MPU6050_Gyroscope_2000s:
			DataStruct->Gyro_Mult = (float)1 / MPU6050_GYRO_SENS_2000;
			// no break
		default:
			break;
	}

	/* Return OK */
	return MPU6050_Result_Ok;
}

/**
 * @brief Tout est dans le nom de la fonction
 * @param DataStruct: pointeur vers la structure où vont être stockées les données
 * @return Message de réussite de l'opération
 */
MPU6050_Result_t MPU6050_ReadAccelerometer(MPU6050_t* DataStruct) {
	uint8_t data[6];
	BSP_I2C_ReadMulti(MPU6050_I2C, DataStruct->Address, MPU6050_ACCEL_XOUT_H, data, 6);

	/* On met au bon format */
	DataStruct->Accelerometer_X = (int16_t)(data[0] << 8 | data[1]);
	DataStruct->Accelerometer_Y = (int16_t)(data[2] << 8 | data[3]);
	DataStruct->Accelerometer_Z = (int16_t)(data[4] << 8 | data[5]);

	return MPU6050_Result_Ok;
}

/**
 * @brief Tout est dans le nom de la fonction
 * @param DataStruct: pointeur vers la structure où vont être stockées les données
 * @return Message de réussite de l'opération
 */
MPU6050_Result_t MPU6050_ReadGyroscope(MPU6050_t* DataStruct) {
	uint8_t data[6];
	BSP_I2C_ReadMulti(MPU6050_I2C, DataStruct->Address, MPU6050_GYRO_XOUT_H, data, 6);

	/* On met au bon format */
	DataStruct->Gyroscope_X = (int16_t)(data[0] << 8 | data[1]);
	DataStruct->Gyroscope_Y = (int16_t)(data[2] << 8 | data[3]);
	DataStruct->Gyroscope_Z = (int16_t)(data[4] << 8 | data[5]);

	return MPU6050_Result_Ok;
}

/**
 * @brief Tout est dans le nom de la fonction
 * @param DataStruct: pointeur vers la structure où vont être stockées les données
 * @return Message de réussite de l'opération
 */
MPU6050_Result_t MPU6050_ReadTemperature(MPU6050_t* DataStruct) {
	uint8_t data[2];
	int16_t temp;
	BSP_I2C_ReadMulti(MPU6050_I2C, DataStruct->Address, MPU6050_TEMP_OUT_H, data, 2);

	/* On met au bon format */
	temp = (int16_t)(data[0] << 8 | data[1]);
	DataStruct->Temperature = (float)((int16_t)temp / (float)340.0 + (float)36.53);

	return MPU6050_Result_Ok;
}

/**
 * @brief Tout est dans le nom de la fonction
 * @param DataStruct: pointeur vers la structure où vont être stockées les données
 * @return Message de réussite de l'opération
 */
MPU6050_Result_t MPU6050_ReadAll(MPU6050_t* DataStruct) {
	uint8_t data[14];
	int16_t temp;

	/* On lit toute la ligne de données, 14bytes */
	BSP_I2C_ReadMulti(MPU6050_I2C, DataStruct->Address, MPU6050_ACCEL_XOUT_H, data, 14);

	/* On met au bon format et on remplie la structure*/
	DataStruct->Accelerometer_X = (int16_t)(data[0] << 8 | data[1]);
	DataStruct->Accelerometer_Y = (int16_t)(data[2] << 8 | data[3]);
	DataStruct->Accelerometer_Z = (int16_t)(data[4] << 8 | data[5]);

	temp = (int16_t)(data[6] << 8 | data[7]);
	DataStruct->Temperature = (float)((float)((int16_t)temp) / (float)340.0 + (float)36.53);

	DataStruct->Gyroscope_X = (int16_t)(data[8] << 8 | data[9]);
	DataStruct->Gyroscope_Y = (int16_t)(data[10] << 8 | data[11]);
	DataStruct->Gyroscope_Z = (int16_t)(data[12] << 8 | data[13]);

	return MPU6050_Result_Ok;
}

/**
 * @brief Fonction de démo pour prendre en main le capteur rapidement.
 * @pre /!\ Cette fonction est blocante /!\
 */
void MPU6050_demo(void){

	MPU6050_t MPU6050_Data;
	int32_t gyro_x = 0;
	int32_t gyro_y = 0;
	int32_t gyro_z = 0;

	/* Initialise le MPU6050 */
	if (MPU6050_Init(&MPU6050_Data, GPIOA, GPIO_PIN_0, MPU6050_Device_0, MPU6050_Accelerometer_8G, MPU6050_Gyroscope_2000s) != MPU6050_Result_Ok) {
		/*
		// Affiche error avec le debug_printf
		debug_printf("MPU6050 Error\n");
		*/

		// Affiche error avec l'UART
		printf("MPU6050 Error\n");

		// Boucle infinie
		while (1);
	}
	while (1) {
		// On
		MPU6050_ReadAll(&MPU6050_Data);

		gyro_x += MPU6050_Data.Gyroscope_X;
		gyro_y += MPU6050_Data.Gyroscope_Y;
		gyro_z += MPU6050_Data.Gyroscope_Z;
		/*
		// Affiche avec le debug_printf
		debug_printf("AX%4d\tAY%4d\tAZ%4d\tGX%4d\tGY%4d\tGZ%4d\tgx%4ld\tgy%4ld\tgz%4ld\tT%3.1f\n",
						MPU6050_Data.Accelerometer_X/410,	//environ en %
						MPU6050_Data.Accelerometer_Y/410,	//environ en %
						MPU6050_Data.Accelerometer_Z/410,	//environ en %
						MPU6050_Data.Gyroscope_X,
						MPU6050_Data.Gyroscope_Y,
						MPU6050_Data.Gyroscope_Z,
						gyro_x/16400,						//environ en °
						gyro_y/16400,						//environ en °
						gyro_z/16400,						//environ en °
						MPU6050_Data.Temperature);

		*/
		 // Affiche avec l'UART
		 printf("AX%4d\tAY%4d\tAZ%4d\tGX%4d\tGY%4d\tGZ%4d\tgx%4ld\tgy%4ld\tgz%4ld\tT%3d\n",
					    (int16_t)MPU6050_Data.Accelerometer_X/410,	//environ en %
					    (int16_t)MPU6050_Data.Accelerometer_Y/410,	//environ en %
					    (int16_t)MPU6050_Data.Accelerometer_Z/410,	//environ en %
					    (int16_t)MPU6050_Data.Gyroscope_X,
					    (int16_t)MPU6050_Data.Gyroscope_Y,
					    (int16_t)MPU6050_Data.Gyroscope_Z,
						(int32_t)gyro_x/16400,						//environ en °
						(int32_t)gyro_y/16400,						//environ en °
						(int32_t)gyro_z/16400,						//environ en °
						(int16_t)MPU6050_Data.Temperature);



		// Un petit délai pour éviter d'avoir un raz-de-marée d'information
		HAL_Delay(500);
	}
}

#endif


