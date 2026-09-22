/**
 *******************************************************************************
 * @file	stm32g4_ld19_display.c
 * @author	vchav
 * @date	Jun 14, 2024
 * @brief	Module pour afficher les trames reçus du capteur lidar ld19
 *******************************************************************************
 * @note 	Ce module propose une méthode pour filtrer et afficher les points détectés et récupérés dans le module stm32g4_ld19.c/.h.
 *          Cette méthode a été choisie car elle est efficace en temps de calcule et elle s'adapte facilement à un maximum de situations.
 *          Toutefois, en fonction de vos besoins, vous pourrez juger que cette méthode n'est pas assez efficace.
 *          Ce qu'il faut retenir de cette méthode, c'est que vous devez
 *          adapter la valeur de BUFFER_DISPLAY_SIZE en fonction de la quantité de choses que devrait capter
 *          le lidar dans la zone que vous aurez définie avec ANGLE_MAX, ANGLE_MIN et SCALE.
 *          Plus BUFFER_DISPLAY_SIZE est bien calibré, plus l'affichage et l'évolution seront fluides.
 */

#include "config.h"
#if USE_LD19
#include "stm32g4_ld19_display.h"
#include "stm32g4_ld19.h"
#include "TFT_ili9341/stm32g4_ili9341.h"
#include "TFT_ili9341/stm32g4_fonts.h"
#include "TFT_ili9341/stm32g4_xpt2046.h"
#include "QS_maths.h"
#include "stdio.h"

#define DISPLAY_SIZE			15 		// 150 pixels
#define SCALE					5 		// m
#define ANGLE_MAX				18000 	// 180 °
#define ANGLE_MIN				0		// 0 °
#define BUFFER_DISPLAY_SIZE 	450		// 450 points

typedef struct{
	int32_t x;
	int32_t y;
}coordinate_t;

typedef struct{
	int16_t distance;
	int16_t angle;
}point_t;

coordinate_t LD19_find_coordinate(uint16_t d, uint16_t a);
void LD19_filter_trame_by_length(point_t * tab_of_point_filtered_1, point_t * tab_of_point_filtered_2);
void LD19_filter_trame_by_angle(point_t * tab_of_point, point_t * tab_of_point_filtered_1);
void LD19_draw_scene(void);
void LD19_init_buffer_displayed(void);

coordinate_t buffer_displayed[BUFFER_DISPLAY_SIZE];
coordinate_t init_point = {0,0};
uint8_t scale = SCALE;
bool init = false;

void BSP_LD19_display_on_tft(ld19_frame_handler_t *frame){

	if(!init)
		BSP_LD19_init_tft();

	static uint16_t i = 0; //compteur pour affichage des constantes
	static uint16_t j = 0; //compteur pour le remplissage du buffer_displayed
	static uint16_t k = 0; //compteur pour la détection du tactile
	static bool duplicate_point = false;

	float angle_step = (frame->end_angle - frame->start_angle)/(POINT_PER_PACK-1);
	coordinate_t coord = init_point;

	//On traite chaque point
	for (int i = 0; i < POINT_PER_PACK; i++){
		if(	frame->point[i].distance < 1000*scale				&&
		    frame->point[i].distance > 50						&&			//On filtre les points
		    frame->start_angle + (i+1)*angle_step < ANGLE_MAX	&&
		    frame->start_angle + (i+1)*angle_step > ANGLE_MIN )
		{
			coord = LD19_find_coordinate(frame->point[i].distance, (frame->start_angle + (i+1)*angle_step));

			if(coord.x < 320 && coord.x > 0 && coord.y < 240 && coord.y > 0){ //On s'assure que les coordonnées rentrent dans l'écran
				for(int i = 0; i<BUFFER_DISPLAY_SIZE; i++){
					if(coord.x == buffer_displayed[i].x && coord.y == buffer_displayed[i].y){ //On regarde si le point existe déjà
						duplicate_point = true;
					}
				}

				if(!duplicate_point){
					ILI9341_DrawPixel(buffer_displayed[j].x, buffer_displayed[j].y, ILI9341_COLOR_WHITE);
					ILI9341_DrawPixel(coord.x, coord.y, ILI9341_COLOR_BLACK);
					buffer_displayed[j]=coord;
					j = (j+1)%BUFFER_DISPLAY_SIZE;
				}
				duplicate_point = false;
			}else{
				//Ne devrait jamais arriver
			}
		}
	}

	//Affichage des constantes tous les 50 trames
	i = (i+1)%50;
	if(i==1){
		char speedtxt[15]; // 15 comme ça on est large
		char scaletxt[20]; // 20 comme ça on est large
		// Convertir entier en chaîne de caractères
		sprintf(speedtxt, "%d degrees/sec", frame->speed);
		sprintf(scaletxt, "150 pixels = %dm", scale);
		ILI9341_Puts(125, 200, speedtxt, &Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
		ILI9341_Puts(125, 220, scaletxt, &Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	}

	//Changement d'échelle si appuie
	k = (k+1)%10; // Afin d'éviter les changements d'échelle trop rapide
	int16_t x, y;
	if(!HAL_GPIO_ReadPin(PIN_IRQ_TOUCH) && k==1){
		XPT2046_getMedianCoordinates(&x, &y, XPT2046_COORDINATE_SCREEN_RELATIVE);
		if (x>240 && y>190){
			scale = (scale-1<1)?(scale):(scale-1);
		}
		else if (x<80 && y>190){
			scale = (scale+1>10)?(scale):(scale+1);
		}
	}

	/*
	printf("\nAprès le filtre: \n");
	for (int i = 0; i < POINT_PER_PACK; i++) {
		if(tab_of_point_filtered_2[i].distance)
			printf("Point %d: Distance = %d, Angle = %d\n", i, tab_of_point_filtered_2[i].distance, tab_of_point_filtered_2[i].angle);
	}*/
}

void LD19_init_buffer_displayed(void){
	for(uint16_t i = 0; i<BUFFER_DISPLAY_SIZE; i++){
		buffer_displayed[i]=init_point;
	}
}

/**
 * @brief Fonction qui filtre les points en fonction de l'échelle (scale) définie.
 * @param tab_of_point: tableau des points à filtrer
 * @param tab_of_point_filtered: tableau qui va contenir les points après le filtre
 */
void LD19_filter_trame_by_length(point_t *tab_of_point, point_t *tab_of_point_filtered){
	uint8_t j = 0;
	for(int i = 0; i < POINT_PER_PACK; i++){
		if(tab_of_point[i].distance < 1000*scale){
			tab_of_point_filtered[j]=tab_of_point[i];
			j++;
		}
	}
}

/**
 * @brief Fonction qui filtre les points en fonction de la fourchette d'angle définie par ANGLE_MAX et ANGLE_MIN.
 * @param tab_of_point: tableau des points à filtrer
 * @param tab_of_point_filtered: tableau qui va contenir les points après le filtre
 */
void LD19_filter_trame_by_angle(point_t *tab_of_point, point_t *tab_of_point_filtered){
	uint8_t j = 0;
	for(int i = 0; i < POINT_PER_PACK; i++){
		if(tab_of_point[i].angle < ANGLE_MAX && tab_of_point[i].angle > ANGLE_MIN){
			tab_of_point_filtered[j]=tab_of_point[i];
			j++;
		}
	}
}

/**
 * @brief Cette fontion renvoie les coordonnées d'un point dont on connais la distance et l'angle
 * @param d: la distance en mm
 * @param a: l'angle en degrés
 * @return les coordonnées
 * @verbatim
 *	+---------------------------+
 *	|
 *	|	  (x;y)
 *	|	 +
 *	|	 |\
 *	|	 | \
 *	|	 |	\ D
 *	|	B|	 \			-> on cherche x et y,
 *	|    |    \			   Et comme on a D et l'angle a et qu'on se souvient de ses cours de troisième,
 *	|	 |	   \		   Il suffit de faire de la trigo: B = sin(a)*D  A = cos(a)*D
 *	|	 |	  a \ 		   Et x = X-A ; y = B
 *	|	 | A   ( \
 *	+-------------X-------------+
 *	@endverbatim
*/
coordinate_t LD19_find_coordinate(uint16_t d, uint16_t a){
	coordinate_t coord;
	int16_t cosinus, sinus;
	uint16_t angle_rad_4096 = GEOMETRY_modulo_angle((int16_t)((uint32_t)a*183/256));
	COS_SIN_4096_get(angle_rad_4096, &cosinus, &sinus);
	coord.x = 160 + ((int32_t)cosinus * d * DISPLAY_SIZE) / (scale*100*4096);
	coord.y = ((int32_t)sinus * d * DISPLAY_SIZE) / (scale*100*4096)  + 40;
	//printf("Coordonnée: x->%d ; y->%d\n",coord.x,coord.y);
	return coord;
}


void BSP_LD19_init_tft(void){
	ILI9341_Init();
	XPT2046_init();
	LD19_init_buffer_displayed();

	ILI9341_Rotate(ILI9341_Orientation_Landscape_2);
	ILI9341_DisplayOff();
	ILI9341_DisplayOn();

	LD19_draw_scene();

	//Les carrés pour zoomer
	ILI9341_DrawFilledRectangle(315, 235, 240, 190, ILI9341_COLOR_GREEN);
	ILI9341_DrawRectangle(315, 235, 240, 190, ILI9341_COLOR_BLACK);
	ILI9341_DrawFilledRectangle(80, 235, 5, 190, ILI9341_COLOR_RED);
	ILI9341_DrawRectangle(80, 235, 5, 190, ILI9341_COLOR_BLACK);

	//Texte:
	ILI9341_Puts(270, 202, "+", &Font_16x26, ILI9341_COLOR_WHITE, ILI9341_COLOR_GREEN);
	ILI9341_Puts(35, 202, "-", &Font_16x26, ILI9341_COLOR_WHITE, ILI9341_COLOR_RED);
	ILI9341_Puts(83, 200, "speed: 0", &Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	ILI9341_Puts(83, 220, "scale: 0", &Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);

	init = true;
}

/**
 * @brief 	Cette fonction affiche la zone d'affichage des points sur l'écran tft.
 * @note	Vous pouvez utiliser cette fonction effacer les points sans casser tous les éléments de l'affichage
 */
void LD19_draw_scene(void){
	//Demi cercle:
	ILI9341_DrawFilledCircle(160, 40, 150, ILI9341_COLOR_WHITE);
	ILI9341_DrawCircle(160, 40, 150, ILI9341_COLOR_BLACK);
	ILI9341_DrawFilledRectangle(320, 40, 300, 0, ILI9341_COLOR_WHITE);
	ILI9341_DrawFilledRectangle(20, 40, 0, 0, ILI9341_COLOR_WHITE);
	ILI9341_DrawLine(10, 40, 310, 40, ILI9341_COLOR_BLACK);
	//Image LD19:
	ILI9341_DrawFilledCircle(160,18,16,ILI9341_COLOR_BLACK); // Rond noir central
	ILI9341_DrawFilledRectangle(185, 22, 135, 14, ILI9341_COLOR_BLACK); // Petites pattes
	ILI9341_DrawFilledCircle(180,18,2,ILI9341_COLOR_WHITE); // Trou de patte
	ILI9341_DrawFilledCircle(140,18,2,ILI9341_COLOR_WHITE); // Trou de patte
	ILI9341_DrawCircle(160,18,16,ILI9341_COLOR_RED);
	ILI9341_Puts(166, 15, ">", &Font_7x10, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK);
	//Echelle:
	ILI9341_DrawLine(160, 37, 310, 37, ILI9341_COLOR_BLUE2);
	ILI9341_Puts(205, 28, "150 pixels", &Font_7x10, ILI9341_COLOR_BLUE2, ILI9341_COLOR_WHITE);
}


#endif
