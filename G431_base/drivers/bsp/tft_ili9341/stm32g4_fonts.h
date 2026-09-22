/**
 * @author  Tilen Majerle
 * @email   tilen@majerle.eu
 * @website http://stm32f4-discovery.com
 * @link    
 * @version v1.2
 * @ide     Keil uVision
 * @license GNU GPL v3
 * @brief   Fonts library for LCD libraries
 *	
@verbatim
   ----------------------------------------------------------------------
    Copyright (C) Tilen Majerle, 2015
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.
     
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
@endverbatim
 */
#ifndef FONTS_H
#define FONTS_H 120

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

#include "config.h"


#include "stm32g4xx.h"
#include "string.h"

/**
 * @brief  Structure de police
 */
typedef struct {
	uint8_t FontWidth;		/*!< Largeur de la police en pixels */
	uint8_t FontHeight;		/*!< Largeur de la police en pixels */
	const void *data;		/*!< Pointeur vers le tableau de données de la police */
	uint8_t datasize;		/*!< nombre d'octets nécessaires pour stocker chaque donnée */
} FontDef_t;

/** 
 * @brief  Longueur et hauteur de la chaîne
 */
typedef struct {
	uint16_t Length;      /*!< Longueur de la chaîne en unités de pixels */
	uint16_t Height;      /*!< Longueur de la chaîne en unités de pixels */
} FONTS_SIZE_t;

/**
 * @brief  Structure de police de taille 7 x 10 pixels
 */
#if USE_FONT7x10
extern FontDef_t Font_7x10;
#endif
/**
 * @brief  Structure de police de taille 11 x 18 pixels
 */
#if USE_FONT11x18
extern FontDef_t Font_11x18;
#endif
/**
 * @brief  Structure de police de taille 16 x 26 pixels
 */
#if USE_FONT16x26
extern FontDef_t Font_16x26;
#endif

/**
 * @brief  Calcule la longueur et la hauteur de la chaîne en unités de pixels en fonction de la chaîne et de la police utilisée
 * @param  *str: Chaîne à vérifier pour la longueur et la hauteur
 * @param  *SizeStruct: Pointeur vers une structure vide @ref FONTS_SIZE_t où les informations seront sauvegardées
 * @param  *Font: Pointeur vers la police @ref FontDef_t utilisée pour les calculs
 * @return Pointeur vers la chaîne utilisée pour la longueur et la hauteur
 */
char* FONTS_GetStringSize(char* str, FONTS_SIZE_t* SizeStruct, FontDef_t* Font);


/* C++ detection */
#ifdef __cplusplus
}
#endif

 
#endif

