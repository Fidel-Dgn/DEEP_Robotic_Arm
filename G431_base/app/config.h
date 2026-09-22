/**
 *******************************************************************************
 * @file 	config.h
 * @author 	jjo
 * @date 	Mar 29, 2024
 * @brief	Fichier principal de configuration de votre projet sur carte Nucléo STM32G431KB.
 * 			Permet d'activer les différents modules logiciels à votre disposition.
 *******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CONFIG_H_
#define CONFIG_H_


/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Defines -------------------------------------------------------------------*/
#define LED_GREEN_PIN 		GPIO_PIN_8
#define LED_GREEN_GPIO 		GPIOB

#define UART2_ON_PA3_PA2
#define UART1_ON_PA10_PA9

#define USE_BSP_TIMER		1
#define USE_BSP_EXTIT		1

#define USE_RTC				0

#define USE_ADC				0
	/* Configuration pour activer les entrées analogiques souhaitées */
	#define USE_IN1		1 //Broche correspondante: PA0
	#define USE_IN2		1 //Broche correspondante: PA1
	#define USE_IN3		0 //Broche correspondante: PA6
	#define USE_IN4		0 //Broche correspondante: PA7
	#define USE_IN10	0 //Broche correspondante: PF1 /!\ Broche indisponible sur g431 sauf si SB11 et pas SB10 (petits pads à souder sur la carte)
	#define USE_IN13	0 //Broche correspondante: PA5
	#define USE_IN17	0 //Broche correspondante: PA4

#define USE_DAC				0

/*------------------Afficheurs------------------*/
#define USE_ILI9341			1 // �cran TFT
#if USE_ILI9341
	#define USE_XPT2046			1
	#define USE_FONT7x10		1
	#define USE_FONT11x18		1
	#define USE_FONT16x26		1
#endif

#define USE_EPAPER			0 // e-paper (�cran basse consommation)
#define USE_WS2812			0 // Matrice de led

/*------------------Capteurs------------------*/
#define USE_MPU6050			0 // Acc�l�rom�tre, Gyroscope
#define USE_APDS9960		1 // Capteur de mouvements, pr�sence, couleurs
#define USE_BMP180			0 // Capteur de pression atmosph�rique
#define USE_BH1750FVI		0 // Capteur de luminosit� ambiante
#define USE_DHT11			0 // Capteur de temp�rature et d'humidit�
#define USE_DS18B20			0 // Sonde de temp�rature
#define USE_YX6300			0 // Lecteur MP3
#define USE_MATRIX_KEYBOARD 0 // Clavier matriciel
#define USE_HCSR04			0 // T�l�metre � ultrason
#define USE_GPS				0 // GPS
#define USE_LD19			0 // Lidar --> veuillez aussi activer USE_ILI9341, USE_XPT2046, USE_FONT7x10 et USE_FONT16x26 si vous voulez utiliser display_ld19.c
#define USE_NFC03A1			0 // Shield NFC
#define USE_VL53L0			0 // T�l�metre laser de grande pr�cision
#if USE_VL53L0
	#define VL53L0X_I2C			I2C1
	#define VL53_NB				1 // La version actuelle du driver ne peut traiter que 8 capteurs au maximum
	//Vous devez d�finir ci dessous autant de lignes VL_x que le nombre indiqu� ci-dessus avec x allant de 0 à VL53_NB-1
	#define VL_0				GPIOA, GPIO_PIN_0 // Pin de reset
	//#define VL_1				GPIOD, GPIO_PIN_3
	//#define VL_2				GPIOE, GPIO_PIN_10
#endif

/*------------------Expanders------------------*/
#define USE_MCP23017		0 // GPIO expander qui communique en i2C
#define USE_MCP23S17		0 // GPIO expander qui communique en SPI
#define USE_SD_CARD			0 // Carte SD pour lire/�crire des fichiers

/*------------------Actionneurs------------------*/
#define USE_MOTOR_DC		0

/*------------------P�riph�riques------------------*/

#if USE_MLX90614 || USE_MPU6050	|| USE_APDS9960	 || USE_BH1750FVI || USE_BMP180 || USE_MCP23017 || USE_VL53L0 // Liste des modules utilisants le périphérique I2C
	#define USE_I2C				1
#else
	#ifndef USE_I2C
		#define USE_I2C				0
	#endif
#endif
#define I2C_TIMEOUT			5//ms

#if USE_ILI9341 || USE_SD_CARD || USE_MCP23S17 || USE_EPAPER // Liste des modules utilisants le p�riph�rique SPI
	#define USE_SPI				1
#else
	#ifndef USE_SPI
		#define USE_SPI				0
	#endif
#endif

#define USE_TESTBOARD				0


#endif /* CONFIG_H_ */
