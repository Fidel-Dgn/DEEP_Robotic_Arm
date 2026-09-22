/**
 *******************************************************************************
 * @file	stm32g4_ili9341.h
 * @author	vchav
 * @date	May 22, 2024
 * @brief	Ce module permet d'utiliser l'écran TFT ili9341. Il est inspiré des
 * 			travaux de Tilen Majerle
 *******************************************************************************
 * @verbatim
 * |----------------------------------------------------------------------
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
 * |----------------------------------------------------------------------
 * @endverbatim
 */

#include "config.h"
#if USE_ILI9341

#ifndef ILI9341_H
#define ILI9341_H 130

#include <TFT_ili9341/stm32g4_fonts.h>
#include "stm32g4xx.h"
#include "stm32g4xx_hal.h"

/**
 * @brief  Les pins du SPI sur stm32g431
 */
#ifndef ILI9341_SPI
	#define ILI9341_SPI           	SPI1
	#define ILI9341_SPI_PORT		GPIOA
	#define ILI9341_SPI_MISO_PIN	GPIO_PIN_6
	#define ILI9341_SPI_MOSI_PIN	GPIO_PIN_7
	#define ILI9341_SPI_SCK_PIN		GPIO_PIN_5
#endif

/**
 * @brief  Chip Select pour le SPI sur stm32g431
 */
#ifndef ILI9341_CS_PIN
#define ILI9341_CS_PORT       GPIOA
#define ILI9341_CS_PIN        GPIO_PIN_4
#endif

#ifndef PIN_CS_TOUCH
	#define PIN_CS_TOUCH       GPIOA, GPIO_PIN_11
#endif

/**
 * @brief  WRX = DC de l'écran sur stm32g431
 */
#ifndef ILI9341_WRX_PIN
#define ILI9341_WRX_PORT      GPIOA
#define ILI9341_WRX_PIN       GPIO_PIN_8
#endif

/**
 * @brief  RESET de l'écran sur stm32g431
 */
#ifndef ILI9341_RST_PIN
#define ILI9341_RST_PORT      GPIOB
#define ILI9341_RST_PIN       GPIO_PIN_3
#endif

/* Paramètres de l'écran */
#ifndef ILI9341_WIDTH
#define ILI9341_WIDTH        240
#endif
#ifndef ILI9341_HEIGHT
#define ILI9341_HEIGHT       320
#endif

#define ILI9341_PIXEL        (ILI9341_HEIGHT*ILI9341_WIDTH)

/* Couleurs */
#define ILI9341_COLOR_WHITE			0xFFFF
#define ILI9341_COLOR_BLACK			0x0000
#define ILI9341_COLOR_RED			0xF800
#define ILI9341_COLOR_GREEN			0x07E0
#define ILI9341_COLOR_GREEN2		0xB723
#define ILI9341_COLOR_BLUE			0x001F
#define ILI9341_COLOR_BLUE2			0x051D
#define ILI9341_COLOR_YELLOW		0xFFE0
#define ILI9341_COLOR_ORANGE		0xFBE4
#define ILI9341_COLOR_CYAN			0x07FF
#define ILI9341_COLOR_MAGENTA		0xA254
#define ILI9341_COLOR_GRAY			0x7BEF
#define ILI9341_COLOR_BROWN			0xBBCA

/* Fond transparent uniquement pour string et char */
#define ILI9341_TRANSPARENT			0x80000000


/**
 * @brief  Orientations possibles du LCD
 *
 *
 * ILI9341_Orientation_Portrait_1		ILI9341_Orientation_Portrait_2
 *
 * 		O_____________O						* * * * * * * <-- pins
 * 		|             |						O-------------O
 * 		| +-----> x   |						| +-----> x	  |
 * 		| |           |						| |           |
 * 		| |           |						| |           |
 * 		| !           |						| !           |
 * 		| y           |						| y           |
 * 		|             |						|             |
 * 		|             |						|             |
 * 		|             |						|             |
 * 		---------------						O-------------O
 * 		O             O
 * 		 * * * * * * * <-- pins
 *
 *
 *  ILI9341_Orientation_Landscape_1			ILI9341_Orientation_Landscape_2
 *
 *
 * 		 O_______________________O *		 *	O_______________________O
 * 		 | +-----> x      		 | *		 *	| +-----> x      		|
 * 		 | |			   		 | *		 *	| |			   		 	|
 * 		 | |           		  	 | *		 *	| |			   			|
 * 		 | !           		  	 | *		 *	| !           		  	|
 * 		 | y           		  	 | *		 *	| y           		  	|
 * 		 | 	           		  	 | *		 *	| 	           		  	|
 * 		 O-----------------------O *		 *	O-----------------------O
 * 		              		  	   ^		 ^
 * 	   							  pins	   	pins
 *
 */
typedef enum {
	ILI9341_Orientation_Portrait_1,  /*!< Portrait orientation mode 1 */
	ILI9341_Orientation_Portrait_2,  /*!< Portrait orientation mode 2 */
	ILI9341_Orientation_Landscape_1, /*!< Landscape orientation mode 1 */
	ILI9341_Orientation_Landscape_2  /*!< Landscape orientation mode 2 */
} ILI9341_Orientation_t;

/**
 * @brief  LCD options
 */
typedef struct {
	uint16_t width;
	uint16_t height;
	ILI9341_Orientation_t orientation; // 1 = portrait; 0 = landscape
} ILI931_Options_t;


void ILI9341_demo(void);

void ILI9341_Init(void);

void ILI9341_setConfig(void);

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

void ILI9341_Fill(uint16_t color);

void ILI9341_Rotate(ILI9341_Orientation_t orientation);

void ILI9341_Putc(uint16_t x, uint16_t y, char c, FontDef_t* font, uint16_t foreground, uint16_t background);

void ILI9341_PutBigc(uint16_t x, uint16_t y, char c, FontDef_t *font, uint16_t foreground, uint16_t background, uint8_t bigger, uint8_t full_in_bigger);

void ILI9341_Puts(uint16_t x, uint16_t y, char* str, FontDef_t *font, uint16_t foreground, uint16_t background);

void ILI9341_PutBigs(uint16_t x, uint16_t y, char *str, FontDef_t *font, uint16_t foreground, uint16_t background, uint8_t bigger, uint8_t full_in_bigger) ;

void ILI9341_GetStringSize(char* str, FontDef_t* font, uint16_t* width, uint16_t* height);

void ILI9341_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

void ILI9341_DrawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

void ILI9341_DrawFilledRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

void ILI9341_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void ILI9341_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void ILI9341_DisplayOn(void);

void ILI9341_DisplayOff(void);

ILI931_Options_t ILI9341_getOptions(void);

void ILI9341_printf(int16_t x, int16_t y, FontDef_t *font, int16_t foreground, int16_t background, const char *format, ...)  __attribute__((format (printf, 6, 7)));

void ILI9341_putImage(int16_t x0, int16_t y0, int16_t width, int16_t height, const int16_t *img, int32_t size);

void ILI9341_putImage_with_transparency(int16_t x0, int16_t y0, int16_t width, int16_t height, const int16_t *img_front, const int16_t * img_back, int32_t size);

void ILI9341_putImage_monochrome(uint16_t color_front, uint16_t color_background, int16_t x0, int16_t y0, int16_t width, int16_t height, const uint8_t *img, int32_t size);

void ILI9341_putImage_3bits(int16_t x0, int16_t y0, int16_t width, int16_t height, uint8_t *img, int32_t size, uint8_t scale);
/* C++ detection */
#ifdef __cplusplus
}
#endif
#endif
#endif // USE_ILI9341

