/**
 ******************************************************************************
 * @file 	stm32g4_ws2812.c
 * @author 	Samuel Poiraud 	&& Luc Hérault
 * @date 	May 3, 2016		&& 2024 --> portage sur G431
 * @brief 	Module pour controler le ws2812s
 ******************************************************************************
 *
 *@verbatim
 *	les envois de pixels sont sous-traités à la fonction assembleur WS2812S_send_pixel
 *      	https://github.com/Daedaluz/stm32-ws2812/tree/master/src
 *  Sur la matrice de 64 WS2812, les leds sont chainées ligne après ligne.
 *@endverbatim
 *
 */
 
 
#include "config.h"
#if USE_WS2812

#include "stm32g4xx_hal.h"
#include "stm32g4_ws2812.h"
#include "config.h"
#include "stm32g4_utils.h"
#include "stm32g4_gpio.h"
#include "stm32g4_sys.h"

#if !(defined WS2812_PORT_DATA) || !defined(WS2812_PIN_DATA)
	#define WS2812_PORT_DATA	GPIOB
	#define WS2812_PIN_DATA		GPIO_PIN_4
#endif


#define T1H		1
#define T1L		1
#define T0H		0
#define T0L		1
#define RES     200

#define OUTPUT(x)	HAL_GPIO_WritePin(WS2812_PORT_DATA, WS2812_PIN_DATA, x)


void BSP_WS2812_init(void)
{
	BSP_GPIO_pin_config(WS2812_PORT_DATA, WS2812_PIN_DATA, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
}


extern void WS2812_send_pixel_asm(uint32_t pixel, uint32_t gpio_pin_x, uint32_t * gpiox_bsrr);

#define WS2812_send_pixel(pixel) WS2812_send_pixel_asm(pixel, WS2812_PIN_DATA, (uint32_t *)&WS2812_PORT_DATA->BSRR)

/**
 * @brief fonction pour prendre en main le module. Les patriotes vont apprécier cette fonction ;)
 */
void BSP_WS2812_demo(void)
{
	static uint8_t j = 0;
	uint8_t i;
	BSP_WS2812_init();
	while(1)
	{
			uint32_t pixels[64];
			for(i=0;i<64;i++)
			{
				if((i%8) < 2)
					pixels[i] = WS2812_COLOR_LIGHT_RED;
				else if((i%8) < 5)
					pixels[i] = WS2812_COLOR_LIGHT_WHITE;
				else
					pixels[i] = WS2812_COLOR_LIGHT_BLUE;
			}
			pixels[j] = WS2812_COLOR_BLACK;
			j = (j+1)%64;

			BSP_WS2812_display(pixels, 64);

			HAL_Delay(50);
	}
}

/**
 * @brief	Cette fonction envoie 64 pixels vers la matrice de leds.
 * @note	les envois de pixels sont sous-traités à la fonction assembleur WS2812S_send_pixel
 * 			Cette fonction est rédigée en assembleur pour respecter scrupuleusement les délais de production des signaux pour les leds de la matrice.
 * 			Remarque : les interruptions sont désactivées temporairement pendant l'exécution de cette fonction pour éviter qu'elles provoquent des 'pauses' lors de la production des signaux.
 * 			La durée d'exécution de cette fonction est de l'ordre de 2,5ms. Durée pendant laquelle aucune interruption ne peut survenir !!!
 * @param 	pixels est un tableau de 64 cases absolument...
 * @note	attention, le tableau de pixels correspond aux leds dans l'ordre où elles sont câblées. Sur la matrice 8x8, elles sont reliées en serpentin ! (et non en recommancant à gauche à chaque nouvelle ligne)...
 */
void BSP_WS2812_display(uint32_t * pixels, uint8_t size)
{
	uint8_t i;
	__disable_irq();
	BSP_WS2812_reset();
	for(i=0;i<size;i++)
		WS2812_send_pixel(pixels[i]);
	__enable_irq();
}

void BSP_WS2812_display_only_one_pixel(uint32_t pixel, uint8_t rank, uint8_t size)
{
	uint8_t i;
	__disable_irq();
	BSP_WS2812_reset();
	for(i=0;i<size;i++)
		WS2812_send_pixel((i==rank)?pixel:WS2812_COLOR_BLACK);
	__enable_irq();
}

void BSP_WS2812_display_full(uint32_t pixel, uint8_t size)
{
	uint8_t i;
	__disable_irq();
	BSP_WS2812_reset();
	for(i=0;i<size;i++)
		WS2812_send_pixel(pixel);
	__enable_irq();
}

void BSP_WS2812_reset(void){

	//int i;
	OUTPUT(0);
	Delay_us(100);
	//for(i = 0; i < RES; i++);	//Utilisez cette fonction et reglée RES si la fonction Delay_us n'est pas disponible.
}


#endif
