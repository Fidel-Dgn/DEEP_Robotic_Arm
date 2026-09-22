/**
 *******************************************************************************
 * @file 	stm32g4_apds9960.c
 * @author 	vchav
 * @date 	May 3, 2024
 * @brief	Module pour l'utilisation du capteur APDS9960
 *******************************************************************************
 */
/**
 * Ce pilote permet d'interfacer le capteur de luminosité RGB et de mouvement APDS9960.
 * Ce capteur permet de mesurer la luminosité ambiante, les luminosités de chacune des trois couleurs RGB, mais également de
 * détecter des mouvements tels qu'un "swipe" vers la droite par exemple ou encore de détecter un objet à proximité.
 * Les modes d'uilisation sont les suivants (et peuvent être activés en même temps) :
 *
 ************************************************************************************************************************
 * 	 1.	Mode "Capteur de luminosité" :
 *
 * 	 	    Permet de mesurer la luminosité ambiante et les niveaux de ses trois composantes RGB.
 *		    Il intègre lui même deux modes :
 *
 *			- Mode "Normal" : permet de faire une simple lecture des valeurs.
 *					1. Initialiser le capteur : init()
 *					2. Activer le capteur en mode capteur de luminosité : enableLightSensor(false)
 *					3. Lire les valeurs désirées : readAmbientLight();
 *												   readRedLight();
 *				                                   readBlueLight();
 *				                                   readGreenLight();
 *				    4. Ajouter un délai(250ms par ex) pour s'assurer que les mesures ont le temps de se faire.
 *
 *			- Mode Interruption : quand la luminosité est comprise dans un certain intervalle définit par l'utilisateur,
 *			  une interruption est déclenchée.
 *			  		1. Initialiser le capteur : init()
 *			  		2. Définir les seuils (inférieur et supérieur) d'interruption : setLightIntLowThreshold()
 *			  																		setLightIntHighThreshold()
 *			  		3. Activer le mode capteur de luminosité en autorisant les interruptions : enableLightSensor(true)
 *			  		4. Gérer les interruptions (initialisation de la broche de l'IT, activation des IT, routine d'interruption, etc).
 *			  		5. --!!!-- Après chaque interruption générée par une luminosité comprise dans l'intervalle défini
 *			  		   précédemment, ne pas oublier d'acquitter l'interruption : clearAmbientLightInt() ---!!!---
 *
 *************************************************************************************************************************
 *	 2.	Mode "Capteur de proximité" :
 *
 * 	 	    Permet de détecter un objet à proximité. Le capteur mesure le "niveau de proximité" qui correspond
 * 	 	    à une valeur sur 8 bits. Plus l'objet est proche, plus la valeur est élevée.
 *		    Tout comme le mode "Capteur de luminosité", il présente deux modes :
 *
 *			- Mode "Normal" : permet de faire une simple lecture du niveau de proximité.
 *					1. Initialiser le capteur : init()
 *					2. Activer le capteur en mode capteur de proximité : enableProximitySensor(false)
 *					3. Lire le niveau de proximité : readProximity()
 *					4. Ajouter un délai(250ms par ex) pour s'assurer que les mesures ont le temps de se faire.
 *
 *			- Mode Interruption : quand le niveau de proximité est compris dans un certain intervalle définit par l'utilisateur,
 *			  une interruption est déclenchée.
 *			  		1. Initialiser le capteur : init()
 *			  		2. Définir les seuils (inférieur et supérieur) d'interruption : setProximityIntLowThreshold()
 *			  																		setProximityIntHighThreshold()
 *			  		3. Activer le mode capteur de proximité en autorisant les interruptions : enableProximitySensor(true)
 *			  		4. Gérer les interruptions (initialisation de la broche de l'IT en mode interruption sur front descendant,
 *			  		   activation des IT, routine d'interruption, etc).
 *			  		5. --!!!-- Après chaque interruption générée par un niveau de proximité compris dans l'intervalle défini
 *			  		   précédemment, ne pas oublier d'acquitter l'interruption : clearProximityInt() ---!!!---
 *
 ***************************************************************************************************************************
 *   3.	Mode "Détecteur de mouvement" :
 *
 * 	 	    Permet de détecter certains gestes de l'utilisateur effectués au-dessus du capteur : balayages droite/gauche,
 * 	 	    gauche/droite, bas/haut, haut/bas, éloignement et rapprochement.
 *
 *					1. Initialiser le capteur : init()
 *					2. Initialiser la broche d'interruption en mode interruption sur front descendant.
 *					2. Activer le capteur en mode détecteur de mouvement en autorisant les IT : enableGestureSensor(true)
 *					3. Activer les interruptions.
 *					4. Créer une fonction permettant de traiter un geste détecté. Cette fonction vérifiera tout d'abord
 *					   la disponibilité d'un geste ( isGestureAvailable() ). Si un geste est bien disponible, on lit
 *					   le geste ( readGesture() ) et on fait l'action désirée en fonction du geste lu. A la fin de cette
 *					   fonction, veiller à REACTIVER LES IT qui ont temporairement été désactivées le temps de traiter
 *					   le geste détecté.
 *					5. Définir la routine d'interruption système, dans laquelle, après avoir acquitté l'IT,
 *					   on DESACTIVERA LES IT temporairement, le temps de traiter le mouvement ayant généré l'IT.
 *					6. Quand une IT a été levée, appeler la méthode de traitement de geste créée lors de l'étape 4
 *					   (dans la tâche de fond par exemple).
 *
 *
 *************************************************************************************************************************
 *
 * Par défaut, les broches utilisées sont celles de l'I2C1 : PA15 pour SCL et PB7 pour SDA.
 * Le basculement sur l'I2C2 ou l'I2C3 peut se faire en modifiant la macro APDS9960_I2C dans le .h.
 * |	I2C2 GPIO Configuration		|		I2C3 GPIO Configuration		|
   |	SDA  --> PF0-OSC_IN			|		SDA  --> PA8				|
   |	SCL  --> PA9				|		SCL  --> PB5				|
 * La broche d'interruption suggérée (INT) est PB6 (vous pouvez la changer par n'importe qu'elle GPIO car la stm32g4 avec les IT externes sur tous ses ports).
 * La broche Vl n'est pas branchée.
 * Le capteur est alimenté par sa broche Vcc en 3.3V.
 */

#include "config.h"
#if USE_APDS9960
#include "stm32g4_apds9960.h"
#include "stm32g4xx_hal.h"
#include "stm32g4_gpio.h"
#include "stm32g4_i2c.h"
#include "stm32g4_uart.h"
#include "stm32g4_utils.h"
#include "stdlib.h"
#include <stdio.h>


/* Private variables ---------------------------------------------------------*/
gesture_data_type gesture_data_;
int gesture_ud_delta_;
int gesture_lr_delta_;
int gesture_ud_count_;
int gesture_lr_count_;
int gesture_near_count_;
int gesture_far_count_;
int gesture_state_;
int gesture_motion_;


/* Private function prototypes -----------------------------------------------*/

/* Gesture processing */
void APDS9960_resetGestureParameters();
bool APDS9960_processGestureData();
bool APDS9960_decodeGesture();

/* Proximity Interrupt Threshold */
uint8_t APDS9960_getProxIntLowThresh();
void APDS9960_setProxIntLowThresh(uint8_t threshold);
uint8_t APDS9960_getProxIntHighThresh();
void APDS9960_setProxIntHighThresh(uint8_t threshold);

/* LED Boost Control */
uint8_t APDS9960_getLEDBoost();
void APDS9960_setLEDBoost(uint8_t boost);

/* Proximity photodiode select */
uint8_t APDS9960_getProxGainCompEnable();
void APDS9960_setProxGainCompEnable(uint8_t enable);
uint8_t APDS9960_getProxPhotoMask();
void APDS9960_setProxPhotoMask(uint8_t mask);

/* Gesture threshold control */
uint8_t APDS9960_getGestureEnterThresh();
void APDS9960_setGestureEnterThresh(uint8_t threshold);
uint8_t APDS9960_getGestureExitThresh();
void APDS9960_setGestureExitThresh(uint8_t threshold);

/* Gesture LED, gain, and time control */
uint8_t APDS9960_getGestureWaitTime();
void APDS9960_setGestureWaitTime(uint8_t time);

/* Gesture mode */
uint8_t APDS9960_getGestureMode();
void APDS9960_setGestureMode(uint8_t mode);

/* Raw I2C Commands */
void wireWriteByte(uint8_t val);
void wireWriteDataByte(uint8_t reg, uint8_t val);
void wireWriteDataBlock(uint8_t reg, uint8_t *val, unsigned int len);
void wireReadDataByte(uint8_t reg, uint8_t *val);
void wireReadDataBlock(uint8_t reg, uint8_t *val, unsigned int len);




/* Function definitions -----------------------------------------------*/

bool APDS9960_init() {
    uint8_t id;

    gesture_ud_delta_ = 0;
	gesture_lr_delta_ = 0;

	gesture_ud_count_ = 0;
	gesture_lr_count_ = 0;

	gesture_near_count_ = 0;
	gesture_far_count_ = 0;

	gesture_state_ = 0;
	gesture_motion_ = DIR_NONE;

    /* Initialize I2C */
    BSP_I2C_Init(APDS9960_I2C, STANDARD_MODE, true);

    /* Read ID register and check against known values for APDS-9960 */
    wireReadDataByte(APDS9960_ID, &id);

    if( !(id == APDS9960_ID_1 || id == APDS9960_ID_2) ) {
        return false;
    }

    /* Set ENABLE register to 0 (disable all features) */
    APDS9960_setMode(ALL, OFF);

    /* Set default values for ambient light and proximity registers */
    wireWriteDataByte(APDS9960_ATIME, DEFAULT_ATIME);
    wireWriteDataByte(APDS9960_WTIME, DEFAULT_WTIME);
    wireWriteDataByte(APDS9960_PPULSE, DEFAULT_PROX_PPULSE);
    wireWriteDataByte(APDS9960_POFFSET_UR, DEFAULT_POFFSET_UR);
    wireWriteDataByte(APDS9960_POFFSET_DL, DEFAULT_POFFSET_DL);
    wireWriteDataByte(APDS9960_CONFIG1, DEFAULT_CONFIG1);
    APDS9960_setLEDDrive(DEFAULT_LDRIVE);
    APDS9960_setProximityGain(DEFAULT_PGAIN);
    APDS9960_setAmbientLightGain(DEFAULT_AGAIN);
    APDS9960_setProxIntLowThresh(DEFAULT_PILT);
    APDS9960_setProxIntHighThresh(DEFAULT_PIHT);
    APDS9960_setLightIntLowThreshold(DEFAULT_AILT);
    APDS9960_setLightIntHighThreshold(DEFAULT_AIHT);
    wireWriteDataByte(APDS9960_PERS, DEFAULT_PERS);
    wireWriteDataByte(APDS9960_CONFIG2, DEFAULT_CONFIG2);
    wireWriteDataByte(APDS9960_CONFIG3, DEFAULT_CONFIG3);

    /* Set default values for gesture sense registers */
    APDS9960_setGestureEnterThresh(DEFAULT_GPENTH);
    APDS9960_setGestureExitThresh(DEFAULT_GEXTH);
    wireWriteDataByte(APDS9960_GCONF1, DEFAULT_GCONF1);
    APDS9960_setGestureGain(DEFAULT_GGAIN);
    APDS9960_setGestureLEDDrive(DEFAULT_GLDRIVE);
    APDS9960_setGestureWaitTime(DEFAULT_GWTIME);
    wireWriteDataByte(APDS9960_GOFFSET_U, DEFAULT_GOFFSET);
    wireWriteDataByte(APDS9960_GOFFSET_D, DEFAULT_GOFFSET);
    wireWriteDataByte(APDS9960_GOFFSET_L, DEFAULT_GOFFSET);
    wireWriteDataByte(APDS9960_GOFFSET_R, DEFAULT_GOFFSET);
    wireWriteDataByte(APDS9960_GPULSE, DEFAULT_GPULSE);
    wireWriteDataByte(APDS9960_GCONF3, DEFAULT_GCONF3);
    APDS9960_setGestureIntEnable(DEFAULT_GIEN);


    /* Gesture config register dump */
    uint8_t reg;
    uint8_t val;

    for(reg = 0x80; reg <= 0xAF; reg++) {
        if( (reg != 0x82) && \
            (reg != 0x8A) && \
            (reg != 0x91) && \
            (reg != 0xA8) && \
            (reg != 0xAC) && \
            (reg != 0xAD) )
        {
            wireReadDataByte(reg, &val);
//            Serial.print(reg, HEX);
//            Serial.print(": 0x");
//            Serial.println(val, HEX);
        }
    }

    for(reg = 0xE4; reg <= 0xE7; reg++) {
        wireReadDataByte(reg, &val);
//        Serial.print(reg, HEX);
//        Serial.print(": 0x");
//        Serial.println(val, HEX);
    }
    return true;
}

uint8_t APDS9960_getMode() {
    uint8_t enable_value;

    /* Read current ENABLE register */
    wireReadDataByte(APDS9960_ENABLE, &enable_value);

    return enable_value;
}

/*
 * @brief Cette demo initialise le capteur APDS9960 et effectue périodiquement une lecture des lumières Rouge, Verte et Bleu.
 * @post  Attention, cette fonction de démonstration est blocante et nécéssite un UART pour afficher les données !
 */
void APDS9960_demo_RGB(void)
{
	APDS9960_init();
	APDS9960_enableLightSensor(false);
	//APDS9960_setLEDDrive(1);
	while(1)
	{
		uint16_t ambiant, red, blue, green;
		APDS9960_readAmbientLight(&ambiant);
		APDS9960_readRedLight(&red);
		APDS9960_readBlueLight(&blue);
		APDS9960_readGreenLight(&green);


		if(ambiant)
		{
			blue = (uint16_t)(((uint32_t)blue*100)/ambiant);
			red = (uint16_t)(((uint32_t)red*100)/ambiant);
			green = (uint16_t)(((uint32_t)green*100)/ambiant);
		}
			printf("A:%d\tR:%d\tG:%d\tB:%d\n",ambiant, red, green, blue);
		HAL_Delay(500);

	}
}
/* ------------------------------------------------------------- */



/* ----------------------Setting functions---------------------- */
/**
 * @brief Activé ou désactivé un mode sur l'APDS-9960
 * @param mode: celui à configurer
 * @param enable: on l'active (1) ou on le désactive (0)
 */
void APDS9960_setMode(uint8_t mode, uint8_t enable) {
    uint8_t reg_val;

    /* Read current ENABLE register */
    reg_val = APDS9960_getMode();

    /* Change bit(s) in ENABLE register */
    enable = enable & 0x01;
    if( mode >= 0 && mode <= 6 ) {
        if (enable) {
            reg_val |= (1 << mode);
        } else {
            reg_val &= ~(1 << mode);
        }
    } else if( mode == ALL ) {
        if (enable) {
            reg_val = 0x7F;
        } else {
            reg_val = 0x00;
        }
    }

    /* Write value back to ENABLE register */
    wireWriteDataByte(APDS9960_ENABLE, reg_val);
}

/**
 * @brief Allume l'APDS-9960
 */
void APDS9960_enablePower() {
	APDS9960_setMode(POWER, 1);
}

/**
 * @brief Eteint l'APDS-9960
 */
void APDS9960_disablePower() {
	APDS9960_setMode(POWER, 0);
}

/**
 * @brief Active le mode capteur de lumière ambiante (R/G/B/Ambient).
 * @param interrupts: true pour activer l’interruption externe matérielle sur haute ou basse luminosité
 */

void APDS9960_enableLightSensor(bool interrupts) {
    /* Set default gain, interrupts, enable power, and enable sensor */
    APDS9960_setAmbientLightGain(DEFAULT_AGAIN);
    if(interrupts) {
        APDS9960_setAmbientLightIntEnable(1);
    } else {
        APDS9960_setAmbientLightIntEnable(0);
    }
    APDS9960_enablePower();
    APDS9960_setMode(AMBIENT_LIGHT, 1);
}

/**
 * @brief Désactive le mode capteur de lumière ambiante
 */
void APDS9960_disableLightSensor() {
	APDS9960_setAmbientLightIntEnable(0);
	APDS9960_setMode(AMBIENT_LIGHT, 0);
}

/**
 * @brief Active le mode capteur de proximité
 * @param interrupts: true pour activer l’interruption externe matérielle de proximité
 */
void APDS9960_enableProximitySensor(bool interrupts) {
    /* Set default gain, LED, interrupts, enable power, and enable sensor */
    APDS9960_setProximityGain(DEFAULT_PGAIN);
    APDS9960_setLEDDrive(DEFAULT_LDRIVE);
    if( interrupts ) {
        APDS9960_setProximityIntEnable(1);
    } else {
        APDS9960_setProximityIntEnable(0);
    }
    APDS9960_enablePower();
    APDS9960_setMode(PROXIMITY, 1);
}

/**
 * @brief Désactive le mode capteur de proximité
 */
void APDS9960_disableProximitySensor() {
	APDS9960_setProximityIntEnable(0);
	APDS9960_setMode(PROXIMITY, 0);
}

/**
 * @brief Active le mode reconnaissance de geste
 * @param interrupts: true pour activer l’interruption externe matérielle de geste
 */
void APDS9960_enableGestureSensor(bool interrupts) {
	/* Enable gesture mode
	   Set ENABLE to 0 (power off)
	   Set WTIME to 0xFF
	   Set AUX to LED_BOOST_300
	   Enable PON, WEN, PEN, GEN in ENABLE
	*/
	APDS9960_resetGestureParameters();
	wireWriteDataByte(APDS9960_WTIME, 0xFF);
	wireWriteDataByte(APDS9960_PPULSE, DEFAULT_GESTURE_PPULSE);
	APDS9960_setLEDBoost(LED_BOOST_300);
	if( interrupts ) {
		APDS9960_setGestureIntEnable(1);
	} else {
		APDS9960_setGestureIntEnable(0);
	}
	APDS9960_setGestureMode(1);
	APDS9960_enablePower();
	APDS9960_setMode(WAIT, 1);
	APDS9960_setMode(PROXIMITY, 1);
	APDS9960_setMode(GESTURE, 1);
}

/**
 * @brief Désactive le mode reconnaissance de geste/mouvement
 */
void APDS9960_disableGestureSensor() {
    APDS9960_resetGestureParameters();
    APDS9960_setGestureIntEnable(0);
    APDS9960_setGestureMode(0);
    APDS9960_setMode(GESTURE, 0);
}

/**
* @brief Réinitialise tous les paramètres du membre de données de mouvement
 */
void APDS9960_resetGestureParameters() {
    gesture_data_.index = 0;
    gesture_data_.total_gestures = 0;

    gesture_ud_delta_ = 0;
    gesture_lr_delta_ = 0;

    gesture_ud_count_ = 0;
    gesture_lr_count_ = 0;

    gesture_near_count_ = 0;
    gesture_far_count_ = 0;

    gesture_state_ = 0;
    gesture_motion_ = DIR_NONE;
}

/**
 * @brief Paramètre le seuil bas de détection de proximité
 * @param threshold: le seuil de proximité bas
 */
void APDS9960_setProxIntLowThresh(uint8_t threshold)
{
    wireWriteDataByte(APDS9960_PILT, threshold);
}

/**
 * @brief Paramètre le seuil haut de détection de proximité
 * @param threshold: le seuil de proximité haut
 */
void APDS9960_setProxIntHighThresh(uint8_t threshold)
{
    wireWriteDataByte(APDS9960_PIHT, threshold);
}

/**
 * @brief Définit la puissance de la LED pour la proximité et l'ALS
 *
 * Valeur   Courant de la LED
 *   0        100 mA
 *   1         50 mA
 *   2         25 mA
 *   3         12,5 mA
 *
 * @param drive: la valeur (0-3) pour la puissance de la LED
 */
void APDS9960_setLEDDrive(uint8_t drive)
{
    uint8_t val;

    /* Read value from CONTROL register */
    wireReadDataByte(APDS9960_CONTROL, &val);

    /* Set bits in register to given value */
    drive &= 0b00000011;
    drive = drive << 6;
    val &= 0b00111111;
    val |= drive;

    /* Write register value back into CONTROL register */
    wireWriteDataByte(APDS9960_CONTROL, val);
}

/**
 * @brief Définit le gain du récepteur pour la détection de proximité
 *
 * Valeur   Gain
 *   0       1x
 *   1       2x
 *   2       4x
 *   3       8x
 *
 * @param drive: la valeur (0-3) pour le gain
 */
void APDS9960_setProximityGain(uint8_t drive)
{
    uint8_t val;

    /* Read value from CONTROL register */
    wireReadDataByte(APDS9960_CONTROL, &val);

    /* Set bits in register to given value */
    drive &= 0b00000011;
    drive = drive << 2;
    val &= 0b11110011;
    val |= drive;

    /* Write register value back into CONTROL register */
    wireWriteDataByte(APDS9960_CONTROL, val);
}

/**
 * @brief Définit le gain du récepteur pour le capteur de lumière ambiante (ALS)
 *
 * Valeur   Gain
 *   0       1x
 *   1       4x
 *   2      16x
 *   3      64x
 *
 * @param drive: la valeur (0-3) pour le gain
 */
void APDS9960_setAmbientLightGain(uint8_t drive)
{
    uint8_t val;

    /* Read value from CONTROL register */
    wireReadDataByte(APDS9960_CONTROL, &val);

    /* Set bits in register to given value */
    drive &= 0b00000011;
    val &= 0b11111100;
    val |= drive;

    /* Write register value back into CONTROL register */
    wireWriteDataByte(APDS9960_CONTROL, val);
}

/**
 * @brief Définit la valeur d'amplification du courant de la LED
 *
 * Valeur   Courant d'amplification
 *   0        100%
 *   1        150%
 *   2        200%
 *   3        300%
 *
 * @param boost: la valeur (0-3) pour l'amplification du courant (100-300%)
 */
void APDS9960_setLEDBoost(uint8_t boost)
{
    uint8_t val;

    /* Read value from CONFIG2 register */
    wireReadDataByte(APDS9960_CONFIG2, &val);

    /* Set bits in register to given value */
    boost &= 0b00000011;
    boost = boost << 4;
    val &= 0b11001111;
    val |= boost;

    /* Write register value back into CONFIG2 register */
    wireWriteDataByte(APDS9960_CONFIG2, val);
}

/**
 * @brief Définit l'activation de la compensation du gain de proximité
 * @param enable: 1 pour activer la compensation. 0 pour désactiver la compensation.
 */
void APDS9960_setProxGainCompEnable(uint8_t enable)
{
   uint8_t val;

   /* Read value from CONFIG3 register */
   wireReadDataByte(APDS9960_CONFIG3, &val);

   /* Set bits in register to given value */
   enable &= 0b00000001;
   enable = enable << 5;
   val &= 0b11011111;
   val |= enable;

   /* Write register value back into CONFIG3 register */
   wireWriteDataByte(APDS9960_CONFIG3, val);
}

/**
 * @brief Définit le masque pour activer/désactiver les photodiodes de proximité
 * 1 = désactivé, 0 = activé
 * Bit    Photodiode
 *  3       HAUT
 *  2       BAS
 *  1       GAUCHE
 *  0       DROITE
 *
 * @param mask: valeur de masque de 4 bits
 */
void APDS9960_setProxPhotoMask(uint8_t mask)
{
    uint8_t val;

    /* Read value from CONFIG3 register */
    wireReadDataByte(APDS9960_CONFIG3, &val);

    /* Set bits in register to given value */
    mask &= 0b00001111;
    val &= 0b11110000;
    val |= mask;

    /* Write register value back into CONFIG3 register */
    wireWriteDataByte(APDS9960_CONFIG3, val);
}

/**
 * @brief Définit le seuil d'entrée de proximité pour la détection de gestes
 * @param threshold: valeur de proximité nécessaire pour démarrer le mode de détection de gestes
 */
void APDS9960_setGestureEnterThresh(uint8_t threshold)
{
    wireWriteDataByte(APDS9960_GPENTH, threshold);
}

/**
 * @brief Définit le seuil de sortie de proximité pour la détection de gestes
 * @param threshold: valeur de proximité nécessaire pour terminer le mode de détection de gestes
 */
void APDS9960_setGestureExitThresh(uint8_t threshold)
{
    wireWriteDataByte(APDS9960_GEXTH, threshold);
}

/**
 * @brief Définit le gain de la photodiode en mode geste
 * Valeur    Gain
 *   0       1x
 *   1       2x
 *   2       4x
 *   3       8x
 *
 * @param[in] gain la valeur du gain de la photodiode
 */
void APDS9960_setGestureGain(uint8_t gain)
{
    uint8_t val;

    /* Read value from GCONF2 register */
    wireReadDataByte(APDS9960_GCONF2, &val);

    /* Set bits in register to given value */
    gain &= 0b00000011;
    gain = gain << 5;
    val &= 0b10011111;
    val |= gain;

    /* Write register value back into GCONF2 register */
    wireWriteDataByte(APDS9960_GCONF2, val);
}

/**
 * @brief Définit le courant d'entraînement des LED en mode geste
 * Valeur    Courant LED
 *   0        100 mA
 *   1         50 mA
 *   2         25 mA
 *   3         12.5 mA
 *
 * @param[in] drive la valeur pour le courant d'entraînement des LED
 */
void APDS9960_setGestureLEDDrive(uint8_t drive)
{
    uint8_t val;

    /* Read value from GCONF2 register */
    wireReadDataByte(APDS9960_GCONF2, &val);

    /* Set bits in register to given value */
    drive &= 0b00000011;
    drive = drive << 3;
    val &= 0b11100111;
    val |= drive;

    /* Write register value back into GCONF2 register */
    wireWriteDataByte(APDS9960_GCONF2, val);
}

/**
 * @brief Définit le temps en mode basse consommation entre les détections de gestes
 * Valeur    Temps d'attente
 *   0        0,0 ms
 *   1        2,8 ms
 *   2        5,6 ms
 *   3        8,4 ms
 *   4       14,0 ms
 *   5       22,4 ms
 *   6       30,8 ms
 *   7       39,2 ms
 *
 * @param time: la valeur pour le temps d'attente
 */
void APDS9960_setGestureWaitTime(uint8_t time)
{
    uint8_t val;

    /* Read value from GCONF2 register */
    wireReadDataByte(APDS9960_GCONF2, &val);

    /* Set bits in register to given value */
    time &= 0b00000111;
    val &= 0b11111000;
    val |= time;

    /* Write register value back into GCONF2 register */
    wireWriteDataByte(APDS9960_GCONF2, val);
}

/**
 * @brief Définit le seuil bas pour les interruptions de lumière ambiante
 * @param threshold: valeur de seuil bas pour déclencher l'interruption
 */
void APDS9960_setLightIntLowThreshold(uint16_t threshold)
{
    uint8_t val_low;
    uint8_t val_high;

    /* Break 16-bit threshold into 2 8-bit values */
    val_low = threshold & 0x00FF;
    val_high = (threshold & 0xFF00) >> 8;

    /* Write low byte */
    wireWriteDataByte(APDS9960_AILTL, val_low);

    /* Write high byte */
    wireWriteDataByte(APDS9960_AILTH, val_high);
}

/**
 * @brief Définit le seuil haut pour les interruptions de lumière ambiante
 * @param threshold: valeur de seuil haut pour déclencher l'interruption
 */
void APDS9960_setLightIntHighThreshold(uint16_t threshold)
 {
     uint8_t val_low;
     uint8_t val_high;

     /* Break 16-bit threshold into 2 8-bit values */
     val_low = threshold & 0x00FF;
     val_high = (threshold & 0xFF00) >> 8;

     /* Write low byte */
     wireWriteDataByte(APDS9960_AIHTL, val_low);

     /* Write high byte */
     wireWriteDataByte(APDS9960_AIHTH, val_high);
 }

/**
 * @brief Définit le seuil bas pour les interruptions de proximité
 * @param threshold: valeur de seuil bas pour déclencher l'interruption
 */
void APDS9960_setProximityIntLowThreshold(uint8_t threshold)
{
    /* Write threshold value to register */
    wireWriteDataByte(APDS9960_PILT, threshold);
}

/**
 * @brief Définit le seuil haut pour les interruptions de proximité
 * @param threshold: valeur de seuil haut pour déclencher l'interruption
 */
void APDS9960_setProximityIntHighThreshold(uint8_t threshold)
{

    /* Write threshold value to register */
    wireWriteDataByte(APDS9960_PIHT, threshold);
}

/**
 * @brief Active ou désactive les interruptions de lumière ambiante
 * @param enable: 1 pour activer les interruptions, 0 pour les désactiver
 */
void APDS9960_setAmbientLightIntEnable(uint8_t enable)
{
    uint8_t val;

    /* Read value from ENABLE register */
    wireReadDataByte(APDS9960_ENABLE, &val);

    /* Set bits in register to given value */
    enable &= 0b00000001;
    enable = enable << 4;
    val &= 0b11101111;
    val |= enable;

    /* Write register value back into ENABLE register */
    wireWriteDataByte(APDS9960_ENABLE, val);
}

/**
 * @brief Active ou désactive les interruptions de proximité
 * @param enable: 1 pour activer les interruptions, 0 pour les désactiver
 */
void APDS9960_setProximityIntEnable(uint8_t enable)
{
    uint8_t val;

    /* Read value from ENABLE register */
    wireReadDataByte(APDS9960_ENABLE, &val);

    /* Set bits in register to given value */
    enable &= 0b00000001;
    enable = enable << 5;
    val &= 0b11011111;
    val |= enable;

    /* Write register value back into ENABLE register */
    wireWriteDataByte(APDS9960_ENABLE, val);
}

/**
 * @brief Active ou désactive les interruptions liées aux gestes
 * @param enable: 1 pour activer les interruptions, 0 pour les désactiver
 */
void APDS9960_setGestureIntEnable(uint8_t enable)
{
    uint8_t val;

    /* Read value from GCONF4 register */
    wireReadDataByte(APDS9960_GCONF4, &val);

    /* Set bits in register to given value */
    enable &= 0b00000001;
    enable = enable << 1;
    val &= 0b11111101;
    val |= enable;

    /* Write register value back into GCONF4 register */
    wireWriteDataByte(APDS9960_GCONF4, val);
}

/**
 * @brief Indique à la machine à états d'entrer ou de sortir de l'état de détection de gestes
 * @param mode: 1 pour entrer dans l'état de détection de gestes, 0 pour en sortir.
 */
void APDS9960_setGestureMode(uint8_t mode)
{
    uint8_t val;

    /* Read value from GCONF4 register */
    wireReadDataByte(APDS9960_GCONF4, &val);

    /* Set bits in register to given value */
    mode &= 0b00000001;
    val &= 0b11111110;
    val |= mode;

    /* Write register value back into GCONF4 register */
    wireWriteDataByte(APDS9960_GCONF4, val);
}

/**
 * @brief Efface l'interruption de lumière ambiante
 */
void APDS9960_clearAmbientLightInt()
{
    uint8_t throwaway;
    wireReadDataByte(APDS9960_AICLEAR, &throwaway);
}

/**
 * @brief Efface l'interruption de proximité
 */
void APDS9960_clearProximityInt(){
    uint8_t throwaway;
    wireReadDataByte(APDS9960_PICLEAR, &throwaway);
}

/* ------------------------------------------------------------- */


/* ----------------------Reading functions---------------------- */
/**
 * @brief Lit le niveau de lumière ambiante (claire) en tant que valeur sur 16 bits
 * @param val: valeur du capteur de lumière.
 */
void APDS9960_readAmbientLight(uint16_t *val) {
    uint8_t val_byte;
    *val = 0;

    /* Lit la valeur du canal clair, registre de poids faible */
    wireReadDataByte(APDS9960_CDATAL, &val_byte);
    *val = val_byte;

    /* Lit la valeur du canal clair, registre de poids fort */
    wireReadDataByte(APDS9960_CDATAH, &val_byte);
    *val = *val + ((uint16_t)val_byte << 8);
}

/**
 * @brief Lit le niveau de lumière rouge en tant que valeur sur 16 bits
 * @param val: valeur du capteur de lumière.
 */
void APDS9960_readRedLight(uint16_t *val) {
    uint8_t val_byte;
    *val = 0;

    /* Lit la valeur du canal clair, registre de poids faible */
    wireReadDataByte(APDS9960_RDATAL, &val_byte);
    *val = val_byte;

    /* Lit la valeur du canal clair, registre de poids fort */
    wireReadDataByte(APDS9960_RDATAH, &val_byte);
    *val = *val + ((uint16_t)val_byte << 8);
}

/**
 * @brief Lit le niveau de lumière verte en tant que valeur sur 16 bits
 * @param val: valeur du capteur de lumière.
 */
void APDS9960_readGreenLight(uint16_t *val) {
    uint8_t val_byte;
    *val = 0;

    /* Lit la valeur du canal clair, registre de poids faible */
    wireReadDataByte(APDS9960_GDATAL, &val_byte);
    *val = val_byte;

    /* Lit la valeur du canal clair, registre de poids fort */
    wireReadDataByte(APDS9960_GDATAH, &val_byte);
    *val = *val + ((uint16_t)val_byte << 8);
}

/**
 * @brief Lit le niveau de lumière bleue en tant que valeur sur 16 bits
 * @param val: valeur du capteur de lumière.
 */
void APDS9960_readBlueLight(uint16_t *val) {
    uint8_t val_byte;
    *val = 0;

    /* Lit la valeur du canal clair, registre de poids faible */
    wireReadDataByte(APDS9960_BDATAL, &val_byte);
    *val = val_byte;

    /* Lit la valeur du canal clair, registre de poids fort */
    wireReadDataByte(APDS9960_BDATAH, &val_byte);
    *val = *val + ((uint16_t)val_byte << 8);
}


/**
 * @brief Lit le niveau de proximité en tant que valeur sur 8 bits
 * @param val: valeur du capteur de proximité.
 */
void APDS9960_readProximity(uint8_t *val) {
    *val = 0;

    /* Read value from proximity data register */
    wireReadDataByte(APDS9960_PDATA, val);
}

/**
 * @brief Traite un événement de geste et renvoie le geste le plus probable
 * @return Nombre correspondant au geste. -1 en cas d'erreur.
 */
int APDS9960_readGesture() {
    uint8_t fifo_level = 0;
    uint8_t bytes_read = 0;
    uint8_t fifo_data[128];
    uint8_t gstatus;
    int motion;
    int i;

    /* Make sure that power and gesture is on and data is valid */
    if( !APDS9960_isGestureAvailable() || !(APDS9960_getMode() & 0b01000001) ) {
        return DIR_NONE;
    }

    while(1){

	/* Wait some time to collect next batch of FIFO data */
	HAL_Delay(FIFO_PAUSE_TIME);

	/* Get the contents of the STATUS register. Is data still valid? */
	wireReadDataByte(APDS9960_GSTATUS, &gstatus);

	/* If we have valid data, read in FIFO */
	if( (gstatus & APDS9960_GVALID) == APDS9960_GVALID ) {

		/* Read the current FIFO level */
		wireReadDataByte(APDS9960_GFLVL, &fifo_level);

#if DEBUG_APDS
		printf("FIFO Level: %d\n", fifo_level);
#endif

		/* If there's stuff in the FIFO, read it into our data block */
		if( fifo_level > 0) {
			wireReadDataBlock(APDS9960_GFIFO_U, (uint8_t*)fifo_data, (fifo_level * 4));

			bytes_read = fifo_level*4;

#if DEBUG_APDS
			printf("FIFO Dump: ");
			for ( i = 0; i < bytes_read; i++ ) {
				printf("%d ", fifo_data[i]);
			}
			printf("\n");
#endif

			/* If at least 1 set of data, sort the data into U/D/L/R */
			if( bytes_read >= 4 ) {
				for( i = 0; i < bytes_read; i += 4 ) {
					gesture_data_.u_data[gesture_data_.index] = fifo_data[i + 0];
					gesture_data_.d_data[gesture_data_.index] = fifo_data[i + 1];
					gesture_data_.l_data[gesture_data_.index] = fifo_data[i + 2];
					gesture_data_.r_data[gesture_data_.index] = fifo_data[i + 3];
					gesture_data_.index++;
					gesture_data_.total_gestures++;
				}

#if DEBUG_APDS
			printf("Up Data: ");
			for ( i = 0; i < gesture_data_.total_gestures; i++ ) {
				printf("%d ", gesture_data_.u_data[i]);
			}
			printf("\n");
#endif

				/* Filter and process gesture data. Decode near/far state */
				if( APDS9960_processGestureData() ) {
					if( APDS9960_decodeGesture() ) {
						//TODO: U-Turn Gestures
#if DEBUG_APDS
						printf("Gesture : %d", gesture_motion_);
#endif
					}
				}

				/* Reset data */
				gesture_data_.index = 0;
				gesture_data_.total_gestures = 0;
			}
		}
	} else {

		/* Determine best guessed gesture and clean up */
		HAL_Delay(FIFO_PAUSE_TIME);
		APDS9960_decodeGesture();
		motion = gesture_motion_;
#if DEBUG_APDS
		printf("END: %d\n", gesture_motion_);
#endif
		APDS9960_resetGestureParameters();
		return motion;
	}
    }
}

/**
 * @brief Lit un octet depuis le périphérique I2C et le registre spécifié
 * @param reg: le registre à lire
 * @param val: la valeur retournée depuis le registre
 */
void wireReadDataByte(uint8_t reg, uint8_t *val) {
	BSP_I2C_Read(APDS9960_I2C, APDS9960_I2C_ADDR<<1, reg, val);
}

/**
 * @brief Lit un bloc (un tableau) d'octets depuis le périphérique I2C et le registre spécifié
 * @param reg: le registre à lire
 * @param val: pointeur vers le début des données
 * @param len: nombre d'octets à lire
 */
void wireReadDataBlock(uint8_t reg, uint8_t *val, unsigned int len) {
	BSP_I2C_ReadMulti(APDS9960_I2C, APDS9960_I2C_ADDR<<1, reg, val, len);
}

/* ------------------------------------------------------------- */



/* ----------------------Writing functions---------------------- */

/**
 * @brief Écrit un octet unique dans le périphérique I2C (sans registre)
 * @param val: la valeur d'un octet à écrire dans le périphérique I2C
 */
void wireWriteByte(uint8_t val) {
	BSP_I2C_WriteNoRegister(APDS9960_I2C, APDS9960_I2C_ADDR<<1, val);
}

/**
 * @brief Écrit un octet unique dans le périphérique I2C et le registre spécifié
 * @param reg: le registre dans le périphérique I2C où écrire
 * @param val: la valeur d'un octet à écrire dans le périphérique I2C
 */
void wireWriteDataByte(uint8_t reg, uint8_t val) {
	BSP_I2C_Write(APDS9960_I2C, APDS9960_I2C_ADDR<<1, reg, val);
}

/**
 * @brief Écrit un bloc (tableau) d'octets dans le périphérique I2C et le registre spécifié
 * @param reg: le registre dans le périphérique I2C où écrire
 * @param val: pointeur vers le début du tableau d'octets de données
 * @param len: la longueur (en octets) des données à écrire
 */
void wireWriteDataBlock(uint8_t reg, uint8_t *val, unsigned int len) {
	BSP_I2C_WriteMulti(APDS9960_I2C, APDS9960_I2C_ADDR<<1, reg, val, len);
}

/* ------------------------------------------------------------- */



/* ----------------------Getting functions---------------------- */
/**
 * @brief Renvoie le seuil bas pour la détection de proximité
 * @return Seuil bas
 */
uint8_t APDS9960_getProxIntLowThresh()
{
    uint8_t val;

    /* Lire la valeur du registre PILT */
    wireReadDataByte(APDS9960_PILT, &val);

    return val;
}

/**
 * @brief Renvoie le seuil haut pour la détection de proximité
 * @return Seuil haut
 */
uint8_t APDS9960_getProxIntHighThresh()
{
    uint8_t val;

    /* Read value from PIHT register */
    wireReadDataByte(APDS9960_PIHT, &val);

    return val;
}

/**
 * @brief Renvoie la puissance d'entraînement des LED pour la proximité et ALS
 * Valeur    Courant LED
 *   0        100 mA
 *   1         50 mA
 *   2         25 mA
 *   3         12.5 mA
 *
 * @return La valeur de la puissance d'entraînement des LED. 0xFF en cas d'échec.
 */
uint8_t APDS9960_getLEDDrive()
{
    uint8_t val;

    /* Read value from CONTROL register */
    wireReadDataByte(APDS9960_CONTROL, &val);

    /* Shift and mask out LED drive bits */
    val = (val >> 6) & 0b00000011;

    return val;
}

/**
 * @brief Renvoie le gain du récepteur pour la détection de proximité
 *
 * Valeur    Gain
 *   0       1x
 *   1       2x
 *   2       4x
 *   3       8x
 *
 * @return La valeur du gain de proximité. 0xFF en cas d'échec.
 */
uint8_t APDS9960_getProximityGain()
{
    uint8_t val;

    /* Read value from CONTROL register */
    wireReadDataByte(APDS9960_CONTROL, &val);

    /* Shift and mask out PDRIVE bits */
    val = (val >> 2) & 0b00000011;

    return val;
}

/**
 * @brief Renvoie le gain du récepteur pour le capteur de lumière ambiante (ALS)
 *
 * Valeur    Gain
 *   0        1x
 *   1        4x
 *   2       16x
 *   3       64x
 *
 * @return La valeur du gain ALS. 0xFF en cas d'échec.
 */
uint8_t APDS9960_getAmbientLightGain()
{
    uint8_t val;

    /* Read value from CONTROL register */
    wireReadDataByte(APDS9960_CONTROL, &val);

    /* Shift and mask out ADRIVE bits */
    val &= 0b00000011;

    return val;
}

/**
 * @brief Obtient la valeur actuelle d'amplification du courant LED
 * Valeur  Courant amplifié
 *   0        100%
 *   1        150%
 *   2        200%
 *   3        300%
 *
 * @return La valeur d'amplification du courant LED. 0xFF en cas d'échec.
 */
uint8_t APDS9960_getLEDBoost()
{
    uint8_t val;

    /* Read value from GEXTH register */
    wireReadDataByte(APDS9960_GEXTH, &val);

    return val;
}

/**
 * @brief Obtient le gain de la photodiode pendant le mode de geste
 * Valeur    Gain
 *   0        1x
 *   1        2x
 *   2        4x
 *   3        8x
 *
 * @return le gain actuel de la photodiode. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getGestureGain()
{
    uint8_t val;

    /* Read value from GCONF2 register */
    wireReadDataByte(APDS9960_GCONF2, &val);

    /* Shift and mask out GGAIN bits */
    val = (val >> 5) & 0b00000011;

    return val;
}

/**
 * @brief Obtient le courant d'entraînement de la LED pendant le mode de geste
 *
 * Valeur    Courant de la LED
 *   0           100 mA
 *   1            50 mA
 *   2            25 mA
 *   3          12,5 mA
 *
 * @return la valeur du courant d'entraînement de la LED. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getGestureLEDDrive()
{
    uint8_t val;

    /* Read value from GCONF2 register */
    wireReadDataByte(APDS9960_GCONF2, &val);

    /* Shift and mask out GLDRIVE bits */
    val = (val >> 3) & 0b00000011;

    return val;
}

/**
 * @brief Obtient le temps en mode basse consommation entre les détections de gestes
 *
 * Valeur    Temps d'attente
 *   0            0 ms
 *   1            2,8 ms
 *   2            5,6 ms
 *   3            8,4 ms
 *   4           14,0 ms
 *   5           22,4 ms
 *   6           30,8 ms
 *   7           39,2 ms
 *
 * @return le temps d'attente actuel entre les gestes. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getGestureWaitTime()
{
    uint8_t val;

    /* Read value from GCONF2 register */
    wireReadDataByte(APDS9960_GCONF2, &val);

    /* Mask out GWTIME bits */
    val &= 0b00000111;

    return val;
}

/**
 * @brief Obtient le seuil bas pour les interruptions de lumière ambiante
 * @param threshold : seuil bas actuel stocké sur le APDS-9960
 */
void APDS9960_getLightIntLowThreshold(uint16_t *threshold)
{
    uint8_t val_byte;
    *threshold = 0;

    /* Lire la valeur depuis le registre de seuil bas de lumière ambiante, octet faible */
    wireReadDataByte(APDS9960_AILTL, &val_byte);
    *threshold = val_byte;

    /* Lire la valeur depuis le registre de seuil bas de lumière ambiante, octet fort */
    wireReadDataByte(APDS9960_AILTH, &val_byte);
    *threshold = *threshold + ((uint16_t)val_byte << 8);
}

/**
 * @brief Obtient le seuil élevé pour les interruptions de lumière ambiante
 * @param threshold : seuil élevé actuel stocké sur le APDS-9960
 */
void APDS9960_getLightIntHighThreshold(uint16_t *threshold)
{
    uint8_t val_byte;
    *threshold = 0;

    /* Lire la valeur depuis le registre de seuil élevé de lumière ambiante, octet faible */
    wireReadDataByte(APDS9960_AIHTL, &val_byte);
    *threshold = val_byte;

    /* Lire la valeur depuis le registre de seuil élevé de lumière ambiante, octet fort */
    wireReadDataByte(APDS9960_AIHTH, &val_byte);
    *threshold = *threshold + ((uint16_t)val_byte << 8);
}

/**
 * @brief Obtient le seuil bas pour les interruptions de proximité
 * @param threshold : seuil bas actuel stocké sur le APDS-9960
 */
void APDS9960_getProximityIntLowThreshold(uint8_t *threshold)
{
    *threshold = 0;

    /* Lire la valeur depuis le registre de seuil bas de proximité */
    wireReadDataByte(APDS9960_PILT, threshold);
}

/**
 * @brief Obtient le seuil élevé pour les interruptions de proximité
 * @param threshold : seuil bas actuel stocké sur le APDS-9960
 */
void APDS9960_getProximityIntHighThreshold(uint8_t *threshold)
{
    *threshold = 0;

    /* Lire la valeur depuis le registre de seuil bas de proximité */
    wireReadDataByte(APDS9960_PIHT, threshold);
}

/**
 * @brief Obtient si les interruptions de lumière ambiante sont activées ou non
 * @return 1 si les interruptions sont activées, 0 sinon. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getAmbientLightIntEnable()
{
    uint8_t val;

    /* Lire la valeur depuis le registre ENABLE */
    wireReadDataByte(APDS9960_ENABLE, &val);

    /* Décaler et masquer le bit AIEN */
    val = (val >> 4) & 0b00000001;

    return val;
}

/**
 * @brief Obtient si les interruptions de proximité sont activées ou non
 * @return 1 si les interruptions sont activées, 0 sinon. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getProximityIntEnable()
{
    uint8_t val;

    /* Lire la valeur depuis le registre ENABLE */
    wireReadDataByte(APDS9960_ENABLE, &val);

    /* Décaler et masquer le bit PIEN */
    val = (val >> 5) & 0b00000001;

    return val;
}


/**
 * @brief Obtient si les interruptions de gestes sont activées ou non
 * @return 1 si les interruptions sont activées, 0 sinon. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getGestureIntEnable()
{
    uint8_t val;

    /* Lire la valeur du registre GCONF4 */
    wireReadDataByte(APDS9960_GCONF4, &val);

    /* Décaler et masquer le bit GIEN */
    val = (val >> 1) & 0b00000001;

    return val;
}

/**
 * @brief Indique si la machine d'état des gestes est actuellement en cours d'exécution
 * @return 1 si la machine d'état des gestes est en cours d'exécution, 0 sinon. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getGestureMode()
{
    uint8_t val;

    /* Lire la valeur du registre GCONF4 */
    wireReadDataByte(APDS9960_GCONF4, &val);

    /* Masquer le bit GMODE */
    val &= 0b00000001;

    return val;
}

/**
 * @brief Obtient l'activation de la compensation du gain de proximité
 * @return 1 si la compensation est activée. 0 sinon. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getProxGainCompEnable()
{
    uint8_t val;

    /* Lire la valeur du registre CONFIG3 */
    wireReadDataByte(APDS9960_CONFIG3, &val);

    /* Décaler et masquer les bits PCMP */
    val = (val >> 5) & 0b00000001;

    return val;
}

/**
 * @brief Obtient le masque actuel pour les photodiodes de proximité activées/désactivées
 *
 * 1 = désactivé, 0 = activé
 * Bit    Photodiode
 *  3       UP
 *  2       DOWN
 *  1       LEFT
 *  0       RIGHT
 *
 * @return Masque de proximité actuel pour les photodiodes. 0xFF en cas d'erreur.
 */
uint8_t APDS9960_getProxPhotoMask()
{
    uint8_t val;

    /* Lire la valeur du registre CONFIG3 */
    wireReadDataByte(APDS9960_CONFIG3, &val);

    /* Masquer les bits de masque d'activation des photodiodes */
    val &= 0b00001111;

    return val;
}

/**
 * @brief Obtient le seuil de proximité d'entrée actuel pour la détection des gestes
 * @return Seuil de proximité d'entrée actuel.
 */
uint8_t APDS9960_getGestureEnterThresh()
{
    uint8_t val;

    /* Lire la valeur du registre GPENTH */
    wireReadDataByte(APDS9960_GPENTH, &val);

    return val;
}

/**
 * @brief Obtient le seuil de proximité de sortie actuel pour la détection des gestes
 * @return Seuil de proximité de sortie actuel.
 */
uint8_t APDS9960_getGestureExitThresh()
{
    uint8_t val;

    /* Lire la valeur du registre GEXTH */
    wireReadDataByte(APDS9960_GEXTH, &val);

    return val;
}


/* ------------------------------------------------------------- */



/**
 * @brief Traite les données brutes du geste pour déterminer la direction du balayage
 * @return true si l'état proche ou loin est détecté. false sinon.
 */
bool APDS9960_processGestureData() {
    uint8_t u_first = 0;
    uint8_t d_first = 0;
    uint8_t l_first = 0;
    uint8_t r_first = 0;
    uint8_t u_last = 0;
    uint8_t d_last = 0;
    uint8_t l_last = 0;
    uint8_t r_last = 0;
    int ud_ratio_first;
    int lr_ratio_first;
    int ud_ratio_last;
    int lr_ratio_last;
    int ud_delta;
    int lr_delta;
    int i;

    /* If we have less than 4 total gestures, that's not enough */
    if( gesture_data_.total_gestures <= 4 ) {
        return false;
    }

    /* Check to make sure our data isn't out of bounds */
    if( (gesture_data_.total_gestures <= 32) && (gesture_data_.total_gestures > 0) ) {

        /* Find the first value in U/D/L/R above the threshold */
        for( i = 0; i < gesture_data_.total_gestures; i++ ) {
            if( (gesture_data_.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.d_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.r_data[i] > GESTURE_THRESHOLD_OUT) ) {

                u_first = gesture_data_.u_data[i];
                d_first = gesture_data_.d_data[i];
                l_first = gesture_data_.l_data[i];
                r_first = gesture_data_.r_data[i];
                break;
            }
        }

        /* If one of the _first values is 0, then there is no good data */
        if( (u_first == 0) || (d_first == 0) || (l_first == 0) || (r_first == 0) ) {

            return false;
        }
        /* Find the last value in U/D/L/R above the threshold */
        for( i = gesture_data_.total_gestures - 1; i >= 0; i-- ) {
#if DEBUG_APDS
            printf("Finding last:");
            printf(" U:%d", gesture_data_.u_data[i]);
            printf(" D:%d", gesture_data_.d_data[i]);
            printf(" L:%d", gesture_data_.l_data[i]);
            printf(" R:%d\n", gesture_data_.r_data[i]);
#endif
            if( (gesture_data_.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.d_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.r_data[i] > GESTURE_THRESHOLD_OUT) ) {

                u_last = gesture_data_.u_data[i];
                d_last = gesture_data_.d_data[i];
                l_last = gesture_data_.l_data[i];
                r_last = gesture_data_.r_data[i];
                break;
            }
        }
    }

    /* Calculate the first vs. last ratio of up/down and left/right */
    ud_ratio_first = ((u_first - d_first) * 100) / (u_first + d_first);
    lr_ratio_first = ((l_first - r_first) * 100) / (l_first + r_first);
    ud_ratio_last = ((u_last - d_last) * 100) / (u_last + d_last);
    lr_ratio_last = ((l_last - r_last) * 100) / (l_last + r_last);

#if DEBUG_APDS
    printf("Last Values:");
    printf(" U:%d", u_last);
    printf("Last Values:");
    printf(" D:%d", d_last);
    printf("Last Values:");
    printf(" L:%d", l_last);
    printf("Last Values:");
    printf(" R:%d\n", r_last);

    printf("Ratios:");
    printf(" UD Fi: %d", ud_ratio_first);
    printf(" UD La: %d", ud_ratio_last);
    printf(" LR Fi: %d", lr_ratio_first);
    printf(" LR La: %d\n", lr_ratio_last);
#endif

    /* Determine the difference between the first and last ratios */
    ud_delta = ud_ratio_last - ud_ratio_first;
    lr_delta = lr_ratio_last - lr_ratio_first;

#if DEBUG_APDS
    printf("Deltas:");
    printf(" UD: %d", ud_delta);
    printf(" LR: %d\n", lr_delta);
#endif

    /* Accumulate the UD and LR delta values */
    gesture_ud_delta_ += ud_delta;
    gesture_lr_delta_ += lr_delta;

#if DEBUG_APDS
    printf("Accumulations:");
    printf(" UD: %d", gesture_ud_delta_);
    printf(" LR: %d\n", gesture_lr_delta_);
#endif

    /* Determine U/D gesture */
    if( gesture_ud_delta_ >= GESTURE_SENSITIVITY_1 ) {
        gesture_ud_count_ = 1;
    } else if( gesture_ud_delta_ <= -GESTURE_SENSITIVITY_1 ) {
        gesture_ud_count_ = -1;
    } else {
        gesture_ud_count_ = 0;
    }

    /* Determine L/R gesture */
    if( gesture_lr_delta_ >= GESTURE_SENSITIVITY_1 ) {
        gesture_lr_count_ = 1;
    } else if( gesture_lr_delta_ <= -GESTURE_SENSITIVITY_1 ) {
        gesture_lr_count_ = -1;
    } else {
        gesture_lr_count_ = 0;
    }

    /* Determine Near/Far gesture */
    if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 0) ) {
        if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && \
            (abs(lr_delta) < GESTURE_SENSITIVITY_2) ) {

            if( (ud_delta == 0) && (lr_delta == 0) ) {
                gesture_near_count_++;
            } else if( (ud_delta != 0) || (lr_delta != 0) ) {
                gesture_far_count_++;
            }

            if( (gesture_near_count_ >= 10) && (gesture_far_count_ >= 2) ) {
                if( (ud_delta == 0) && (lr_delta == 0) ) {
                    gesture_state_ = NEAR_STATE;
                } else if( (ud_delta != 0) && (lr_delta != 0) ) {
                    gesture_state_ = FAR_STATE;
                }
                return true;
            }
        }
    } else {
        if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && \
            (abs(lr_delta) < GESTURE_SENSITIVITY_2) ) {

            if( (ud_delta == 0) && (lr_delta == 0) ) {
                gesture_near_count_++;
            }

            if( gesture_near_count_ >= 10 ) {
                gesture_ud_count_ = 0;
                gesture_lr_count_ = 0;
                gesture_ud_delta_ = 0;
                gesture_lr_delta_ = 0;
            }
        }
    }

#if DEBUG_APDS
    printf("UD_CT: %d", gesture_ud_count_);
    printf("LR_CT: %d", gesture_lr_count_);
    printf("NEAR_CT: %d", gesture_near_count_);
    printf("FAR_CT: %d\n", gesture_far_count_);
    printf("----------\n");
#endif

    return false;
}

/**
 * @brief Détermine s'il y a un geste disponible à la lecture
 * @return true si un geste est disponible. false sinon.
 */
bool APDS9960_isGestureAvailable() {
    uint8_t val;

    /* Read value from GSTATUS register */
    wireReadDataByte(APDS9960_GSTATUS, &val);

    /* Shift and mask out GVALID bit */
    val &= APDS9960_GVALID;

    /* Return true/false based on GVALID bit */
    if( val == 1) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Détermine la direction du balayage ou l'état proche/loin
 * @return true si l'événement proche/loin. false sinon.
 */
bool APDS9960_decodeGesture() {
    /* Return if near or far event is detected */
    if( gesture_state_ == NEAR_STATE ) {
        gesture_motion_ = DIR_NEAR;
        return true;
    } else if ( gesture_state_ == FAR_STATE ) {
        gesture_motion_ = DIR_FAR;
        return true;
    }

    /* Determine swipe direction */
    if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 0) ) {
        gesture_motion_ = DIR_UP;
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 0) ) {
        gesture_motion_ = DIR_DOWN;
    } else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 1) ) {
        gesture_motion_ = DIR_RIGHT;
    } else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == -1) ) {
        gesture_motion_ = DIR_LEFT;
    } else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_UP;
        } else {
            gesture_motion_ = DIR_RIGHT;
        }
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == -1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_DOWN;
        } else {
            gesture_motion_ = DIR_LEFT;
        }
    } else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == -1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_UP;
        } else {
            gesture_motion_ = DIR_LEFT;
        }
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_DOWN;
        } else {
            gesture_motion_ = DIR_RIGHT;
        }
    } else {
        return false;
    }

    return true;
}

#endif
