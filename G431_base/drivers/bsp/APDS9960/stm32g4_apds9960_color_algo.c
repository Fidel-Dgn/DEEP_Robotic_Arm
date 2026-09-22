/**
 *******************************************************************************
 * @file 	stm32g4_apds9960_color_algo.c
 * @author 	vchav
 * @date 	May 6, 2024
 * @brief	Ce fichier présente un algorithme visant à rechercher la couleur observée la plus proche parmi des couleurs de références
 * 			(calibrées à la main dans les define ci-dessous).
 *			Pour plus d'explication, contactez samuel.poiraud@eseo.fr après avoir consulté attentivement ce fichier.
 *******************************************************************************
 */
#include "config.h"
#if USE_APDS9960

#include "stm32g4_utils.h"
#include "stm32g4_apds9960.h"

/* Private defines ---------------------------------------------------------- */
#define COLOR_SENSOR_AVERAGE_DIFFRENCIAL_MULTIPLIER			1
#define COLOR_SENSOR_DIFFRENCIAL_DIFFRENCIAL_MULTIPLIER		0

// Red
#define COLOR_SENSOR_RED__RED			75
#define COLOR_SENSOR_RED__GREEN			14
#define COLOR_SENSOR_RED__BLUE			25
#define COLOR_SENSOR_RED__RED_GREEN		(ABSOLUTE(COLOR_SENSOR_RED__RED - COLOR_SENSOR_RED__GREEN))
#define COLOR_SENSOR_RED__RED_BLUE		(ABSOLUTE(COLOR_SENSOR_RED__RED - COLOR_SENSOR_RED__BLUE))
#define COLOR_SENSOR_RED__GREEN_BLUE	(ABSOLUTE(COLOR_SENSOR_RED__GREEN - COLOR_SENSOR_RED__BLUE))

// Green
#define COLOR_SENSOR_GREEN__RED				13
#define COLOR_SENSOR_GREEN__GREEN			51
#define COLOR_SENSOR_GREEN__BLUE			43
#define COLOR_SENSOR_GREEN__RED_GREEN		(ABSOLUTE(COLOR_SENSOR_GREEN__RED - COLOR_SENSOR_GREEN__GREEN))
#define COLOR_SENSOR_GREEN__RED_BLUE		(ABSOLUTE(COLOR_SENSOR_GREEN__RED - COLOR_SENSOR_GREEN__BLUE))
#define COLOR_SENSOR_GREEN__GREEN_BLUE		(ABSOLUTE(COLOR_SENSOR_GREEN__GREEN - COLOR_SENSOR_GREEN__BLUE))

// Blue
#define COLOR_SENSOR_BLUE__RED				13
#define COLOR_SENSOR_BLUE__GREEN			38
#define COLOR_SENSOR_BLUE__BLUE				55
#define COLOR_SENSOR_BLUE__RED_GREEN		(ABSOLUTE(COLOR_SENSOR_BLUE__RED - COLOR_SENSOR_BLUE__GREEN))
#define COLOR_SENSOR_BLUE__RED_BLUE			(ABSOLUTE(COLOR_SENSOR_BLUE__RED - COLOR_SENSOR_BLUE__BLUE))
#define COLOR_SENSOR_BLUE__GREEN_BLUE		(ABSOLUTE(COLOR_SENSOR_BLUE__GREEN - COLOR_SENSOR_BLUE__BLUE))

// White
#define COLOR_SENSOR_WHITE__RED				45 //43
#define COLOR_SENSOR_WHITE__GREEN			32 //60
#define COLOR_SENSOR_WHITE__BLUE			38 //55
#define COLOR_SENSOR_WHITE__AMBIANT			255
#define COLOR_SENSOR_WHITE__RED_GREEN		(ABSOLUTE(COLOR_SENSOR_WHITE__RED - COLOR_SENSOR_WHITE__GREEN))
#define COLOR_SENSOR_WHITE__RED_BLUE		(ABSOLUTE(COLOR_SENSOR_WHITE__RED - COLOR_SENSOR_WHITE__BLUE))
#define COLOR_SENSOR_WHITE__GREEN_BLUE		(ABSOLUTE(COLOR_SENSOR_WHITE__GREEN - COLOR_SENSOR_WHITE__BLUE))

// Black
#define COLOR_SENSOR_BLACK__RED				76 //26
#define COLOR_SENSOR_BLACK__GREEN			21 //37
#define COLOR_SENSOR_BLACK__BLUE			32 //35
#define COLOR_SENSOR_BLACK__AMBIANT			150
#define COLOR_SENSOR_BLACK__RED_GREEN		(ABSOLUTE(COLOR_SENSOR_BLACK__RED - COLOR_SENSOR_BLACK__GREEN))
#define COLOR_SENSOR_BLACK__RED_BLUE		(ABSOLUTE(COLOR_SENSOR_BLACK__RED - COLOR_SENSOR_BLACK__BLUE))
#define COLOR_SENSOR_BLACK__GREEN_BLUE		(ABSOLUTE(COLOR_SENSOR_BLACK__GREEN - COLOR_SENSOR_BLACK__BLUE))

/* Private types ------------------------------------------------------------ */

typedef enum{
	COLOR_SENSOR_NONE,
	COLOR_SENSOR_RED,
	COLOR_SENSOR_GREEN,
	COLOR_SENSOR_BLUE,
	COLOR_SENSOR_WHITE,
	COLOR_SENSOR_BLACK,
	COLOR_SENSOR_NB
}colorSensor_e;


typedef struct{
	colorSensor_e color;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t ambiant;
	uint8_t proximity;
}colorSensor_s;


/**
 * @brief Fonction qui cherche la plus petite valeur dans un tableau
 * @param values[]: tableau de valeurs à parcourir
 * @param nbValue: nombre de valeurs à parcourir dans le tableau. Si vous ne voulez pas parcourir tout le tableau, nbValue sera inférieur à sa taille.
 * @return indiceMin: la plus petite valeur trouvé
 */
uint16_t APDS9960_searchMin(int32_t values[], uint16_t nbValue){
	uint8_t i, indiceMin = 0;
	for(i=0; i<nbValue; i++){
		if(values[i] < values[indiceMin]){
			indiceMin = i;
		}
	}
	return indiceMin;
}


/**
 * Fonction de reconnaissance de couleurs à partir des composantes R, G et B fournies par le capteur couleur.
 * @param sensor_datas les données du capteur (composantes RGB)
 * @param search les couleurs à rechercher (i.e., les couleurs possibles) sous la forme d'un masque binaire
 * @return color: la couleur déduite
 */
static colorSensor_e SENSOR_analyse_color(void)
{
	uint16_t ambiant, red, blue, green;
	uint16_t redGreen, redBlue, greenBlue;
	uint32_t redFound, greenFound, blueFound,whiteFound,blackFound;
	colorSensor_e color;
	color = COLOR_SENSOR_NONE;


	APDS9960_readAmbientLight(&ambiant);
	APDS9960_readRedLight(&red);
	APDS9960_readBlueLight(&blue);
	APDS9960_readGreenLight(&green);


	if(ambiant)
	{
		//Composantes RGB, ramenés en pourcentage de la mesure de lumière ambiante
		blue = (uint16_t)(((uint32_t)blue*100)/ambiant);
		red = (uint16_t)(((uint32_t)red*100)/ambiant);
		green = (uint16_t)(((uint32_t)green*100)/ambiant);
	}

	if(ambiant)
	{
//		debug_printf("%d, %d, %d, %d\n", red, green, blue, sensors[i].ambiant);
		redGreen = ABSOLUTE(red - green);
		redBlue = ABSOLUTE(red - red);
		greenBlue = ABSOLUTE(green - blue);

		int32_t colors_value[COLOR_SENSOR_NB] = {0};
		colorSensor_e colors_id[COLOR_SENSOR_NB] = {0};
		uint8_t nb = 0;


		redFound = (ABSOLUTE(red - COLOR_SENSOR_RED__RED) + ABSOLUTE(green - COLOR_SENSOR_RED__GREEN) + ABSOLUTE(blue - COLOR_SENSOR_RED__BLUE)) * COLOR_SENSOR_AVERAGE_DIFFRENCIAL_MULTIPLIER
					+ (ABSOLUTE(redGreen - COLOR_SENSOR_RED__RED_GREEN) + ABSOLUTE(redBlue - COLOR_SENSOR_RED__RED_BLUE) + ABSOLUTE(greenBlue - COLOR_SENSOR_RED__GREEN_BLUE)) * COLOR_SENSOR_DIFFRENCIAL_DIFFRENCIAL_MULTIPLIER;
		colors_value[nb] = redFound;
		colors_id[nb] = COLOR_SENSOR_RED;
		nb++;


		greenFound = (ABSOLUTE(red - COLOR_SENSOR_GREEN__RED) + ABSOLUTE(green - COLOR_SENSOR_GREEN__GREEN) + ABSOLUTE(blue - COLOR_SENSOR_GREEN__BLUE)) * COLOR_SENSOR_AVERAGE_DIFFRENCIAL_MULTIPLIER
					+ (ABSOLUTE(redGreen - COLOR_SENSOR_GREEN__RED_GREEN) + ABSOLUTE(redBlue - COLOR_SENSOR_GREEN__RED_BLUE) + ABSOLUTE(greenBlue - COLOR_SENSOR_GREEN__GREEN_BLUE)) * COLOR_SENSOR_DIFFRENCIAL_DIFFRENCIAL_MULTIPLIER;
		colors_value[nb] = greenFound;
		colors_id[nb] = COLOR_SENSOR_GREEN;
		nb++;


		blueFound = (ABSOLUTE(red - COLOR_SENSOR_BLUE__RED) + ABSOLUTE(green - COLOR_SENSOR_BLUE__GREEN) + ABSOLUTE(blue - COLOR_SENSOR_BLUE__BLUE)) * COLOR_SENSOR_AVERAGE_DIFFRENCIAL_MULTIPLIER
					+ (ABSOLUTE(redGreen - COLOR_SENSOR_BLUE__RED_GREEN) + ABSOLUTE(redBlue - COLOR_SENSOR_BLUE__RED_BLUE) + ABSOLUTE(greenBlue - COLOR_SENSOR_BLUE__GREEN_BLUE)) * COLOR_SENSOR_DIFFRENCIAL_DIFFRENCIAL_MULTIPLIER;
		colors_value[nb] = blueFound;
		colors_id[nb] = COLOR_SENSOR_BLUE;
		nb++;


		whiteFound = (ABSOLUTE(ambiant - COLOR_SENSOR_WHITE__AMBIANT) + ABSOLUTE(red - COLOR_SENSOR_WHITE__RED) + ABSOLUTE(green - COLOR_SENSOR_WHITE__GREEN) + ABSOLUTE(blue - COLOR_SENSOR_WHITE__BLUE)) * COLOR_SENSOR_AVERAGE_DIFFRENCIAL_MULTIPLIER
					+ (ABSOLUTE(redGreen - COLOR_SENSOR_WHITE__RED_GREEN) + ABSOLUTE(redBlue - COLOR_SENSOR_WHITE__RED_BLUE) + ABSOLUTE(greenBlue - COLOR_SENSOR_WHITE__GREEN_BLUE)) * COLOR_SENSOR_DIFFRENCIAL_DIFFRENCIAL_MULTIPLIER;
		colors_value[nb] = whiteFound;
		colors_id[nb] = COLOR_SENSOR_WHITE;
		nb++;


		blackFound = (ABSOLUTE(ambiant - COLOR_SENSOR_BLACK__AMBIANT) + ABSOLUTE(red - COLOR_SENSOR_BLACK__RED) + ABSOLUTE(green - COLOR_SENSOR_BLACK__GREEN) + ABSOLUTE(blue - COLOR_SENSOR_BLACK__BLUE)) * COLOR_SENSOR_AVERAGE_DIFFRENCIAL_MULTIPLIER
					+ (ABSOLUTE(redGreen - COLOR_SENSOR_BLACK__RED_GREEN) + ABSOLUTE(redBlue - COLOR_SENSOR_BLACK__RED_BLUE) + ABSOLUTE(greenBlue - COLOR_SENSOR_BLACK__GREEN_BLUE)) * COLOR_SENSOR_DIFFRENCIAL_DIFFRENCIAL_MULTIPLIER;
		colors_value[nb] = blackFound;
		colors_id[nb] = COLOR_SENSOR_BLACK;
		nb++;


		uint8_t indiceMin = APDS9960_searchMin(colors_value, nb);
		color = colors_id[indiceMin];
	}

	return color;
}

#endif
