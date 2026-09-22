/**
 *******************************************************************************
 * @file	stm32g4_xpt2046.c
 * @author	vchav
 * @date	May 23, 2024
 * @brief	Module pour utiliser le tactile XPT2046 du TFT ili9341 sur cible stm32g4
 * 			Adaptation du module stm32f1_xpt2046.c de louisz
 *******************************************************************************
 */

/*
 * Cette bibliothèque permet d'utiliser la partie tactile XPT2046 de l'écran TFT avec n'importe quel STM32G4 et est indépendante de la bibliothèque ILI9341.
 *
 * Le module utilise le bus SPI et les broches sont imposées (l'alimentation adéquate et la GND doivent également être choisies) :
 *
 *  T_IRQ : PB5 : Broche IRQ de l'écran tactile utilisée pour notifier lorsqu'on touche ou relâche l'écran (actuellement non utilisée)
 *  T_OUT : PA6 : Sortie de l'écran vers SPI (MISO)
 *  T_IN  : PA7 : Entrée depuis le maître SPI (MOSI)
 *  T_CS  : PA11: Sélection de puce pour SPI
 *  T_CLK : PA5 : Horloge SPI
 *  VCC   : 5V  : Alimentation
 *  GND   : GND : Masse
 *
 *  Comment utiliser la bibliothèque de l'écran tactile XPT2046 :
 *
 *  - Assurez-vous que USE_XPT2046 dans config.h est correctement défini à 1
 *  - Initialisez la bibliothèque (fonction d'initialisation)
 *  - Obtenez les données de l'écran en utilisant getCoordinate ou getAverageCoordinate (getAverageCoordinate est recommandé pour une meilleure précision)
 *
 */

#include "config.h"
#if USE_XPT2046

#include "stm32g4_xpt2046.h"
#include "stm32g4_spi.h"
#include "stm32g4_ili9341.h"
#include "stm32g4_gpio.h"
#include "stm32g4_utils.h"


// Définition du bit de départ (S n7)
#define CONTROL_BYTE_START                                    0b10000000
#define CONTROL_BYTE_NO_START                                 0b00000000

// Différents bits de sélection des canaux (bits A2-A0 : 6-4)
// Octet de sélection pour la température 0 (contrôle de température)
#define CONTROL_BYTE_CHANNEL_SELECT_TEMP0                     0b00000000
// Autre entrée de température
#define CONTROL_BYTE_CHANNEL_SELECT_TEMP1                     0b01110000
// Octet de sélection pour Y
#define CONTROL_BYTE_CHANNEL_SELECT_Y                         0b00010000
// Octet de sélection pour X
#define CONTROL_BYTE_CHANNEL_SELECT_X                         0b01010000
// Mesures croisées du panneau
#define CONTROL_BYTE_CHANNEL_SELECT_Z1                        0b00110000
#define CONTROL_BYTE_CHANNEL_SELECT_Z2                        0b01000000
// Octet de sélection pour le moniteur de batterie
#define CONTROL_BYTE_CHANNEL_SELECT_VBAT                      0b00100000
// Entrée auxiliaire
#define CONTROL_BYTE_CHANNEL_SELECT_AUX                       0b01100000

// Choix du mode de conversion (bit MODE n3)
#define CONTROL_BYTE_MODE_12_BIT                              0b00000000
#define CONTROL_BYTE_MODE_8_BIT                               0b00001000

// Choix du mode Single Ended ou différentiel (bit SER/DFR n2)
#define CONTROL_BYTE_SD_DIFFERENTIAL                          0b00000000
#define CONTROL_BYTE_SD_SINGLE_ENDED                          0b00000100

// Sélection du mode de mise hors tension (bits PD1 et PD0 : 1 et 0)
#define CONTROL_BYTE_POWER_DOWN_MODE_LOW_POWER_IRQ            0b00000000
#define CONTROL_BYTE_POWER_DOWN_MODE_REF_ON_ADC_OFF_NO_IRQ    0b00000001
#define CONTROL_BYTE_POWER_DOWN_MODE_REF_OFF_ADC_ON_IRQ       0b00000010
#define CONTROL_BYTE_POWER_DOWN_MODE_FULL_POWER_NO_IRQ        0b00000011

// Type d'octet de contrôle
typedef uint8_t controlByte_t;

#define XPT2046_CS_SET()			HAL_GPIO_WritePin(PIN_CS_TOUCH,GPIO_PIN_SET)
#define XPT2046_CS_RESET()			HAL_GPIO_WritePin(PIN_CS_TOUCH,GPIO_PIN_RESET)

static uint16_t XPT2046_getReading(controlByte_t controlByte);
static void XPT2046_convertCoordinateScreenMode(int16_t * pX, int16_t * pY);


/**
 * @brief Cette fonction a pour but de vous aider à appréhender les fonctionnalités de ce module logiciel.
 * @note /!\ Cette fonction est blocante /!\
 */
void XPT2046_demo(void)
{
	ILI9341_Init();	//initialisation de l'écran TFT
	ILI9341_Rotate(ILI9341_Orientation_Landscape_2);
	ILI9341_Fill(ILI9341_COLOR_WHITE);
	ILI9341_DrawCircle(20,20,5,ILI9341_COLOR_BLUE);
	ILI9341_DrawLine(20,20,100,20,ILI9341_COLOR_RED);
	ILI9341_DrawLine(20,20,20,100,ILI9341_COLOR_RED);

	#if USE_FONT7x10
		ILI9341_Putc(110,11,'x',&Font_7x10,ILI9341_COLOR_BLUE,ILI9341_COLOR_WHITE);
		ILI9341_Putc(15,110,'y',&Font_7x10,ILI9341_COLOR_BLUE,ILI9341_COLOR_WHITE);
	#endif

	XPT2046_init();	//initialisation du tactile

	while(1)
	{
		static int16_t static_x,static_y;
		int16_t x, y;

		if(XPT2046_getMedianCoordinates(&x, &y, XPT2046_COORDINATE_SCREEN_RELATIVE))
		{
			ILI9341_DrawCircle(static_x,static_y,15,ILI9341_COLOR_WHITE); // Pour effacer l'ancien cercle
			ILI9341_DrawCircle(static_x,static_y,16,ILI9341_COLOR_WHITE); // Pour être sur d'avoir bien effacé l'ancien cercle
			ILI9341_DrawCircle(static_x,static_y,14,ILI9341_COLOR_WHITE); // Pour être sur d'avoir bien effacé l'ancien cercle
			ILI9341_DrawCircle(x,y,15,ILI9341_COLOR_BLUE); // Pour dessiner un cercle bleu
			static_x = x;
			static_y = y;
		}
	}
}

/**
 * @brief Fonction d'initialisation du XPT2046
 */
void XPT2046_init(void){

	// Initialise SPI
	BSP_SPI_Init(XPT2046_SPI, FULL_DUPLEX, MASTER, SPI_BAUDRATEPRESCALER_32);
	uint32_t previousBaudrate;
	previousBaudrate = BSP_SPI_getBaudrate(XPT2046_SPI);
	BSP_SPI_setBaudRate(XPT2046_SPI, SPI_BAUDRATEPRESCALER_256);	//slow for XPT2046
	BSP_GPIO_pin_config(PIN_CS_TOUCH,GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
	BSP_GPIO_pin_config(PIN_IRQ_TOUCH,GPIO_MODE_INPUT,GPIO_PULLDOWN,GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
	XPT2046_CS_SET();

	XPT2046_getReading(CONTROL_BYTE_START
					   | CONTROL_BYTE_CHANNEL_SELECT_X
					   | CONTROL_BYTE_MODE_12_BIT
					   | CONTROL_BYTE_SD_DIFFERENTIAL
					   | CONTROL_BYTE_POWER_DOWN_MODE_LOW_POWER_IRQ);

	BSP_SPI_setBaudRate(XPT2046_SPI, previousBaudrate);	//"fast" for everyone else...
}

/**
 * @brief Fonction pour obtenir les coordonnées selon un mode de coordonnées choisi.
 * @param pX: Pointeur pour obtenir la coordonnée X.
 * @param pY: Pointeur pour obtenir la coordonnée Y.
 * @param coordinateMode: Mode de coordonnées, soit XPT2046_COORDINATE_RAW, soit XPT2046_COORDINATE_SCREEN_RELATIVE.
 * @return Un booléen indiquant si l'écran a été touché (et si les coordonnées retournées sont valides).
 */
bool XPT2046_getCoordinates(int16_t * pX, int16_t * pY, XPT2046_coordinateMode_e coordinateMode){
	uint8_t i, j;
	int16_t allX[7] , allY[7];
	bool ret;

	uint32_t previousBaudrate;
	previousBaudrate = BSP_SPI_getBaudrate(XPT2046_SPI);
	BSP_SPI_setBaudRate(XPT2046_SPI, SPI_BAUDRATEPRESCALER_256);	//slow for XPT2046

	for (i=0; i < 7 ; i++){

		allY[i] = (int16_t)XPT2046_getReading(CONTROL_BYTE_START
										| CONTROL_BYTE_CHANNEL_SELECT_Y
										| CONTROL_BYTE_MODE_12_BIT
										| CONTROL_BYTE_SD_DIFFERENTIAL
										| CONTROL_BYTE_POWER_DOWN_MODE_LOW_POWER_IRQ);

		allX[i] = (int16_t)XPT2046_getReading(CONTROL_BYTE_START
										| CONTROL_BYTE_CHANNEL_SELECT_X
										| CONTROL_BYTE_MODE_12_BIT
										| CONTROL_BYTE_SD_DIFFERENTIAL
										| CONTROL_BYTE_POWER_DOWN_MODE_LOW_POWER_IRQ);
	}

	for (i=0; i < 4 ; i++){
		for (j=i; j < 7 ; j++) {
			int16_t temp = allX[i];
			if(temp > allX[j]){
				allX[i] = allX[j];
				allX[j] = temp;
			}
			temp = allY[i];
			if(temp > allY[j]){
				allY[i] = allY[j];
				allY[j] = temp;
			}
		}
	}

#ifdef XPT2046_USE_PIN_IRQ_TO_CHECK_TOUCH
	if(!HAL_GPIO_ReadPin(PIN_IRQ_TOUCH))
		ret = true;
	else
		ret =  false;
#else
	if(allX[3] >= 4000 || allY[3] >= 4000 || allX[3] < 100 || allY[3] < 100)
		ret =  false;
	else
		ret =  true;
#endif

	if(coordinateMode == XPT2046_COORDINATE_SCREEN_RELATIVE)
		XPT2046_convertCoordinateScreenMode(&(allX[3]), &(allY[3]));

	*pX = allX[3];
	*pY = allY[3];

	BSP_SPI_setBaudRate(XPT2046_SPI, previousBaudrate);	//"fast" for everyone else...

	return ret;
}

/**
 * @brief Fonction pour obtenir la coordonnée moyenne dans un mode de coordonnées choisi sur un nombre défini de valeurs.
 * @param pX: Pointeur pour obtenir la coordonnée X.
 * @param pY: Pointeur pour obtenir la coordonnée Y.
 * @param nSamples: Nombre d'échantillons utilisés pour chaque valeur retournée.
 * @param coordinateMode: Soit XPT2046_COORDINATE_RAW, soit XPT2046_COORDINATE_SCREEN_RELATIVE.
 * @return Un booléen indiquant si l'écran a été touché (et si la coordonnée retournée est valide).
 */
bool XPT2046_getAverageCoordinates(int16_t * pX, int16_t * pY, uint8_t nSamples, XPT2046_coordinateMode_e coordinateMode){
	uint8_t i = 0;
	int32_t xAcc = 0 , yAcc = 0;
	int16_t x , y;
	for(i=0; i<nSamples; i++)
	{
		if(XPT2046_getCoordinates(&x , &y, coordinateMode))
		{
			xAcc += x;
			yAcc += y;
		}
		else
			return false;
	}

	if(i < nSamples)
		return false;

	*pX = (int16_t)(xAcc / i);
	*pY = (int16_t)(yAcc / i);
	ILI931_Options_t screenOption = ILI9341_getOptions();
	if(screenOption.orientation == ILI9341_Orientation_Portrait_1 || screenOption.orientation == ILI9341_Orientation_Portrait_2)
	{
		if(*pX > 0 && *pX < 239 && *pY > 0 && *pY < 319)
			return true;
	}
	else
	{
		if(*pX > 0 && *pX < 319 && *pY > 0 && *pY < 239)
			return true;
	}
	return false;
}


#define NB_POINTS_FOR_MEDIAN		8

/**
 * @brief Fonction pour obtenir la coordonnée médiane dans un mode de coordonnées choisi sur un nombre défini de valeurs.
 * @param pX Pointeur: pour obtenir la coordonnée X.
 * @param pY Pointeur: pour obtenir la coordonnée Y.
 * @param coordinateMode: Soit XPT2046_COORDINATE_RAW, soit XPT2046_COORDINATE_SCREEN_RELATIVE.
 * @return Un booléen indiquant si l'écran a été touché (et si la coordonnée retournée est valide).
 */
bool XPT2046_getMedianCoordinates(int16_t * pX, int16_t * pY, XPT2046_coordinateMode_e coordinateMode){
	uint8_t n = 0, i, j;
	typedef struct
	{
		int16_t x;
		int16_t y;
	}point_t;
	point_t tab[NB_POINTS_FOR_MEDIAN];
	point_t current;
	int16_t index;

	index = 0;
	XPT2046_getCoordinates(&tab[0].x , &tab[0].y, coordinateMode);	//on place le premier point dans la première case du tableau.


	for(n=1; n<NB_POINTS_FOR_MEDIAN; n++)
	{
		if(XPT2046_getCoordinates(&current.x , &current.y, coordinateMode))	//récup d'un point
		{
			for(i=0; i<index; i++)		//parcours des valeurs plus faibles
			{
				if(current.x < tab[i].x)
					break;
			}
			for(j=(uint8_t)index; j>i; j--)	//déplacement des valeurs plus grandes de 1 case
			{
				tab[j] = tab[j-1];
			}
			tab[i] = current;	//écriture de la nouvelle valeur à sa position

			index++;
		}
		else
			return false;
	}

	*pX = tab[index/2].x;
	*pY = tab[index/2].y;
	ILI931_Options_t screenOption = ILI9341_getOptions();
	if(screenOption.orientation == ILI9341_Orientation_Portrait_1 || screenOption.orientation == ILI9341_Orientation_Portrait_2)
	{
		if(*pX > 0 && *pX < 239 && *pY > 0 && *pY < 319)
			return true;
	}
	else
	{
		if(*pX > 0 && *pX < 319 && *pY > 0 && *pY < 239)
			return true;
	}
	return false;
}

/**
 * @brief Fonction privée utilisée pour lire le SPI.
 * @param controlByte: Byte de contrôle utilisé pour la lecture.
 * @return Données lues depuis l'écran tactile.
 */
static uint16_t XPT2046_getReading(controlByte_t controlByte){

	uint16_t ret;

	XPT2046_CS_RESET();
	BSP_SPI_WriteNoRegister(XPT2046_SPI,controlByte);

	ret = (uint16_t)((uint16_t)(BSP_SPI_ReadNoRegister(XPT2046_SPI)) << 5);
	ret |= (uint16_t)(BSP_SPI_ReadNoRegister(XPT2046_SPI) >> (uint16_t)(3));

	XPT2046_CS_SET();

	return ret;
}

/**
 * @brief Fonction privée utilisée pour convertir les coordonnées bruts en coordonnées exploitables.
 * @param pX: Coordonnée X.
 * @param pY: Coordonnée Y.
 */
static void XPT2046_convertCoordinateScreenMode(int16_t * pX, int16_t * pY){
	ILI931_Options_t screenOption = ILI9341_getOptions();
	int32_t tempX, tempY;
	tempX = (int32_t)*pX;
	tempY = (int32_t)*pY;

	switch(screenOption.orientation){
	case ILI9341_Orientation_Portrait_1 :
		*pX = (int16_t)((4096 - tempX) * (int32_t)screenOption.width / 4096);
		*pY = (int16_t)((4096 - tempY) * (int32_t)screenOption.height / 4096);
		break;

	case ILI9341_Orientation_Portrait_2 :
		*pX = (int16_t)(tempX * (int32_t)screenOption.width / 4096);
		*pY = (int16_t)(tempY * (int32_t)screenOption.height / 4096);
		break;

	case ILI9341_Orientation_Landscape_1 :
		*pX = (int16_t)((4096 - tempY) * (int32_t)screenOption.width / 4096);
		*pY = (int16_t)(tempX * (int32_t)screenOption.height / 4096);
		break;

	case ILI9341_Orientation_Landscape_2 :
		*pX = (int16_t)((tempY) * (int32_t)screenOption.width / 4096);
		*pY = (int16_t)((4096 - tempX) * (int32_t)screenOption.height / 4096);
		break;
	}
}



#endif

