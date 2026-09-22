/**
 *******************************************************************************
 * @file 	main.c
 * @author 	jjo
 * @date 	Mar 29, 2024
 * @brief	Fichier principal de votre projet sur carte Nucléo STM32G431KB
 *******************************************************************************
 */

#include "config.h"
#include "stm32g4_sys.h"

#include "stm32g4_systick.h"
#include "stm32g4_gpio.h"
#include "stm32g4_uart.h"
#include "stm32g4_utils.h"
#include "APDS9960/stm32g4_apds9960.h"
#include "stm32g4_timer.h"
#include "tft_ili9341/stm32g4_ili9341.h"
#include "tft_ili9341/stm32g4_fonts.h"
#include "tft_ili9341/stm32g4_xpt2046.h"
#include <servo.h>
#include <stdio.h>

#define PROXIMITY_THRESHOL 150
#define BLINK_DELAY		100	//ms
void TestCharReceiveUart(uint8_t c);
void Test_Ecran(void);
void example_rgb_read(void);


// Couleurs personnalisées pour l'interface
#define COLOR_ESEO          0x0194  // Bleu ESEO
#define COLOR_BG            ILI9341_COLOR_WHITE
#define COLOR_TEXT          ILI9341_COLOR_BLACK
#define COLOR_BAR_BG        0xDEDB  // Gris clair pour le fond des barres


// Positions de la Base (rotation)
#define BASE_INIT_A    90
#define BASE_X_BLEU    45
#define BASE_Y_ROUGE   135
#define BASE_Z_VERT    180

// Positions des autres articulations pour "saisir" (exemples)
#define SHOULDER_POS   60
#define ELBOW_POS      40
#define WRIST_POS      90
#define GRIPPER_OPEN   10
#define GRIPPER_CLOSE  80

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

uint16_t target_base_angle = BASE_INIT_A;



// Suivi de l'état pour rafraîchissement intelligent

uint16_t proximity_bar_width = 0;


/*typedef enum {
    INIT,
    WAIT_APDS,
    DECISION,
    TRIE
} SystemState;


SystemState state = INIT;   // variable globale
SystemState last_state = -1;*/


typedef enum {    // Nouvel état : attend l'appui sur l'écran
    INIT,
    WAIT_APDS,
    DECISION,
    TRIE
} SystemState;

SystemState state = INIT; // On commence en veille
SystemState last_state = -1;

// Structure pour définir la zone du bouton Start
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} TouchButton;

TouchButton startBtn = {80, 140, 160, 60}; // Bouton centré

bool isButtonPressed(int16_t touchX, int16_t touchY, TouchButton btn) {
    return (touchX >= btn.x && touchX <= (btn.x + btn.w) &&
            touchY >= btn.y && touchY <= (btn.y + btn.h));
}



/**
  * @brief  Point d'entrée de votre application
  */
int main(void)
{
	/* Cette ligne doit rester la première de votre main !
	 * Elle permet d'initialiser toutes les couches basses des drivers (Hardware Abstraction Layer),
	 * condition préalable indispensable à l'exécution des lignes suivantes.
	 */
	uint8_t proximity = 0;   // Pour la lecture de proximité (0-255)
	uint16_t r=0, g=0, b=0;  // Pour les lectures de couleurs
	HAL_Init();
	ILI9341_Init();


	/* Initialisation des périphériques utilisés dans votre programme */
	BSP_GPIO_enable();
	BSP_UART_init(UART2_ID,115200);


	/*
	APDS9960_init();
	APDS9960_getMode();
	APDS9960_setMode(ALL, ON);
	APDS9960_enablePower();
	APDS9960_enableLightSensor(interrupts);
	APDS9960_enableProximitySensor(interrupts);*/
	/* Indique que les printf sont dirigés vers l'UART2 */
	BSP_SYS_set_std_usart(UART2_ID, UART2_ID, UART2_ID);


	// Dans ton main, avant le while(1)
	APDS9960_init();
	APDS9960_enablePower();
	// On configure le gain au minimum pour éviter la saturation (8224)
	APDS9960_setAmbientLightGain(AGAIN_1X);




	/* Initialisation du port de la led Verte (carte Nucleo) */
	BSP_GPIO_pin_config(LED_GREEN_GPIO, LED_GREEN_PIN, GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH,GPIO_NO_AF);

	/* Hello student */
	printf("Hi <Student>, can you read me?\n");

	//heartbeat();
	SERVO_init();



	//fonction test Ecran
	//Test_Ecran();
	//ILI9341_Init();
	//ILI9341_demo();
	//ILI9341_Puts(10,200,"OK je foctionne..",&Font_7x10,ILI9341_COLOR_BLACK,ILI9341_COLOR_WHITE);
	//Test Demo APDS
	//APDS9960_demo_RGB();

	//test

	/* Tâche de fond, boucle infinie, Infinite loop,... quelque soit son nom vous n'en sortirez jamais */
//	while (1)
//	{
		//example_rgb_read();

		/*

		SERVO_process_test(TIMER3_ID, TIM_CHANNEL_1);PB4
		SERVO_process_test(TIMER3_ID, TIM_CHANNEL_3);PB0
		//SERVO_process_test(TIMER3_ID, TIM_CHANNEL_4);
		SERVO_process_test(TIMER1_ID, TIM_CHANNEL_3);PA10
		SERVO_process_test(TIMER2_ID, TIM_CHANNEL_1);PA0
		SERVO_process_test(TIMER2_ID, TIM_CHANNEL_2);PA1

		switch (state)
		{
		    case INIT:
		        // Code
		    	APDS9960_enablePower();
		    	APDS9960_enableLightSensor(interrupts);
		    	APDS9960_enableProximitySensor(interrupts);
		        break;

		    case WAIT_APDS:
		        // Code
		        break;

		    case DECISION:
		    	// Code
		    	APDS9960_disablePower();
		    	break;

		    case TRIE:

		    	break;

		    default:
		        // Code si aucun case ne correspond
		        break;
		}
	*/
		while (1)
		{
		    // --- 1. RAFRAÎCHISSEMENT DE L'ÉCRAN (Seulement si l'état change) ---
		    if (state != last_state)
		    {
		        // Effacer la zone de travail centrale
		        ILI9341_DrawFilledRectangle(0, 40, 319, 239, COLOR_BG);

		        // Dessiner le Header (Bandeau titre)
		        ILI9341_DrawFilledRectangle(0, 0, 319, 35, COLOR_ESEO);
		        ILI9341_Puts(10, 8, "ROBOTIC ARM PROJECT - ESEO", &Font_11x18, ILI9341_COLOR_WHITE, COLOR_ESEO);

		        // Afficher le nom de l'état actuel en gros
		        char state_str[20];
		        switch(state) {
		            case INIT:      sprintf(state_str, "INITIALISATION"); break;
		            case WAIT_APDS: sprintf(state_str, "ATTENTE OBJET "); break;
		            case DECISION:  sprintf(state_str, "ANALYSE...     "); break;
		            case TRIE:      sprintf(state_str, "TRI EN COURS   "); break;
		        }
		        ILI9341_Puts(10, 50, state_str, &Font_16x26, ILI9341_COLOR_BLUE, COLOR_BG);

		        last_state = state;
		    }

		    // --- 2. LOGIQUE ET AFFICHAGE DYNAMIQUE ---
		    switch (state)
		    {
		        case INIT:
		            ILI9341_Puts(10, 100, "Calibration capteurs...", &Font_7x10, COLOR_TEXT, COLOR_BG);
		            APDS9960_init();
		            APDS9960_enablePower();
		            APDS9960_enableProximitySensor(false);
		            APDS9960_enableLightSensor(false);
		            HAL_Delay(1000);
		            state = WAIT_APDS;
		            break;

		        case WAIT_APDS:
		            APDS9960_readProximity(&proximity);

		            // Affichage d'une jauge de proximité
		            ILI9341_Puts(10, 100, "Distance objet:", &Font_7x10, COLOR_TEXT, COLOR_BG);
		            ILI9341_DrawRectangle(10, 115, 210, 130, ILI9341_COLOR_BLACK); // Contour
		            ILI9341_DrawFilledRectangle(11, 116, 209, 129, COLOR_BAR_BG);  // Fond

		            // Largeur proportionnelle (proximity va de 0 à 255)
		            uint16_t bar_w = (proximity * 198) / 255;
		            ILI9341_DrawFilledRectangle(11, 116, 11 + bar_w, 129, ILI9341_COLOR_BLUE);

		            if (proximity > PROXIMITY_THRESHOL) {
		                state = DECISION;
		            }
		            break;

		        case DECISION:
		                    // 1. Lecture des capteurs
		                    APDS9960_readRedLight(&r);
		                    APDS9960_readGreenLight(&g);
		                    APDS9960_readBlueLight(&b);

		                    // 2. Affichage des valeurs brutes pour le débug
		                    ILI9341_printf(10, 100, &Font_7x10, ILI9341_COLOR_RED, COLOR_BG, "Rouge: %d  ", r);
		                    ILI9341_printf(10, 115, &Font_7x10, ILI9341_COLOR_GREEN, COLOR_BG, "Vert : %d  ", g);
		                    ILI9341_printf(10, 130, &Font_7x10, ILI9341_COLOR_BLUE, COLOR_BG, "Bleu : %d  ", b);

		                    // Contour du carré de prévisualisation
		                    ILI9341_DrawRectangle(220, 90, 300, 150, ILI9341_COLOR_BLACK);

		                    // 3. Logique de décision de couleur
		                    // Test ROUGE
		                    if (r > g && r > b) {
		                        ILI9341_DrawFilledRectangle(221, 91, 299, 149, ILI9341_COLOR_RED);
		                        ILI9341_Puts(10, 160, "RESULTAT: ROUGE", &Font_11x18, ILI9341_COLOR_RED, COLOR_BG);
		                        HAL_Delay(10000);
		                        target_base_angle = BASE_Y_ROUGE;
		                        state = TRIE;
		                    }
		                    // Test VERT
		                    else if (g > r && g > b) {
		                        ILI9341_DrawFilledRectangle(221, 91, 299, 149, ILI9341_COLOR_GREEN);
		                        ILI9341_Puts(10, 160, "RESULTAT: VERT ", &Font_11x18, ILI9341_COLOR_GREEN, COLOR_BG);
		                        HAL_Delay(10000);

		                        target_base_angle = BASE_Z_VERT;
		                        state = TRIE;
		                    }
		                    // --- NOUVEAU : Test BLEU ---
		                    else if (b > r && b > g) {
		                        ILI9341_DrawFilledRectangle(221, 91, 299, 149, ILI9341_COLOR_BLUE);
		                        ILI9341_Puts(10, 160, "RESULTAT: BLEU  ", &Font_11x18, ILI9341_COLOR_BLUE, COLOR_BG);
		                        HAL_Delay(10000);

		                        target_base_angle = BASE_X_BLEU;
		                        state = TRIE;
		                    }
		                    // Si aucune couleur ne domine clairement ou si c'est trop sombre
		                    else {
		                        state = WAIT_APDS;
		                    }
		                    break;

		        case TRIE:

		       	   ILI9341_Puts(10, 200, "Tri en cours...", &Font_7x10, ILI9341_COLOR_GRAY, COLOR_BG);
		       	   //SERVO_set_angle(TIMER3_ID, TIM_CHANNEL_1, BASE_INIT_A);
		       	   SERVO_set_angle(TIMER2_ID, TIM_CHANNEL_2, GRIPPER_CLOSE);
		       	 /*
		            // --- ÉTAPE 1 : ALLER CHERCHER L'OBJET (Position A) ---
		            // On s'assure que la base est bien à A (90°)
		            SERVO_set_angle(TIMER3_ID, TIM_CHANNEL_1, BASE_INIT_A);
		            // On baisse le bras et on ferme la pince
		            SERVO_set_angle(TIMER3_ID, TIM_CHANNEL_3, SHOULDER_POS);
		            SERVO_set_angle(TIMER1_ID, TIM_CHANNEL_3, ELBOW_POS);
		            HAL_Delay(500);
		            SERVO_set_angle(TIMER2_ID, TIM_CHANNEL_2, GRIPPER_CLOSE); // Fermer pince
		            HAL_Delay(500);

		            // --- ÉTAPE 2 : PIVOTER VERS LA DESTINATION (X, Y ou Z) ---
		            // On lève un peu le bras pour ne pas traîner l'objet
		            SERVO_set_angle(TIMER3_ID, TIM_CHANNEL_3, 90);
		            HAL_Delay(300);
		            // On tourne la base
		            SERVO_set_angle(TIMER3_ID, TIM_CHANNEL_1, target_base_angle);
		            HAL_Delay(800);

		            // --- ÉTAPE 3 : POSER ET REVENIR ---
		            SERVO_set_angle(TIMER2_ID, TIM_CHANNEL_2, GRIPPER_OPEN); // Ouvrir pince
		            HAL_Delay(500);

		            // REVENIR À LA POSITION INITIALE A (Tout le bras se remet droit)
		            SERVO_set_angle(TIMER3_ID, TIM_CHANNEL_1, BASE_INIT_A);
		            SERVO_set_angle(TIMER3_ID, TIM_CHANNEL_3, 90);
		            SERVO_set_angle(TIMER1_ID, TIM_CHANNEL_3, 90);
		            SERVO_set_angle(TIMER2_ID, TIM_CHANNEL_1, 90);
		            HAL_Delay(800);
		            */

		            state = WAIT_APDS;
		            break;

		        default: state = INIT; break;
		    }

		    HAL_Delay(1000); // Limite le taux de rafraîchissement pour l'I2C/SPI
		}






}



void write_LED(bool b)
	{
		HAL_GPIO_WritePin(LED_GREEN_GPIO, LED_GREEN_PIN, b);
	}

bool char_received(uart_id_t uart_id)
	{
		if( BSP_UART_data_ready(uart_id) )	/* Si un caractère est reçu sur l'UART 2*/
		{
			/* On "utilise" le caractère pour vider le buffer de réception */
			BSP_UART_get_next_byte(uart_id);
			return true;
		}
		else
			return false;
	}

	void heartbeat(void)
	{
		while(! char_received(UART2_ID) )
		{
			write_LED(true);
			HAL_Delay(50);
			write_LED(false);
			HAL_Delay(1500);
		}
	}


	void TestCharReceiveUart(uint8_t c){

			// reception et envoie sur la broche PA5 de la carte nucleo de chaque bit du caractere envoyer par l'uart
			for(int i = 0; i < 8; i++)
				{
					if( (c >> i) & 1 ){

						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
					}else{
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
					}
					HAL_Delay(1); // 1 ms → visible à l’oscillo

				}

			// petit espace entre caractères
			HAL_Delay(5);


	}

	void Test_Ecran(void){
		ILI9341_Init();

		 //Test couleurs
		ILI9341_Fill(ILI9341_COLOR_RED);
		HAL_Delay(500);
		ILI9341_Fill(ILI9341_COLOR_GREEN);
		HAL_Delay(500);
		ILI9341_Fill(ILI9341_COLOR_BLUE);
		HAL_Delay(500);

		// Test dessin
		ILI9341_DrawLine(0,0,200,200,ILI9341_COLOR_WHITE);
		ILI9341_DrawRectangle(10,10,100,60,ILI9341_COLOR_YELLOW);
		ILI9341_DrawFilledCircle(120,160,30,ILI9341_COLOR_RED);

		// Test texte
		ILI9341_Puts(150,50,"OK je foctionne..",&Font_7x10,ILI9341_COLOR_BLACK,ILI9341_COLOR_WHITE);
	}


	// Exemple de lecture RGB basique
	void example_rgb_read(void) {
	    uint16_t ambient, red, green, blue;

	    // NE PAS réactiver le capteur ici, c'est déjà fait dans le main
	    APDS9960_readAmbientLight(&ambient);
	    APDS9960_readRedLight(&red);
	    APDS9960_readGreenLight(&green);
	    APDS9960_readBlueLight(&blue);

	    // Si les valeurs restent à 8224, affiche aussi la Proximité pour voir si le capteur répond
	    printf("A:%d R:%d G:%d B:%d\n", ambient, red, green, blue);

	    HAL_Delay(200); // Laisse souffler l'I2C
	}
