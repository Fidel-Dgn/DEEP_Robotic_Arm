/**
  ******************************************************************************
  * @file    stm32g4_epaper_demo.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/
#include "config.h"
#if USE_EPAPER
#include "stm32g4xx_hal.h"
#include "stdio.h"
#include "stm32g4_uart.h"
#include "stm32g4_sys.h"
#include "stm32g4_utils.h"
#include "stm32g4_gpio.h"
#include "epaper/stm32g4_epaper_com.h"
#include "epaper/stm32g4_epaper_if.h"
#include "epaper/stm32g4_epaper_paint.h"
#include "epaper/stm32g4_epaper_imagedata.h"
#define COLORED      1
#define UNCOLORED    0

/**
 * @brief	fonction pour prendre en main le epaper facilement.
 * @note /!\ Cette fonction est bloquante /!\
 */
void EPAPER_demo(void)
{
	static unsigned char frame_buffer[(EPD_WIDTH * EPD_HEIGHT / 8)];

	EPD epd;
	if (EPD_Init(&epd) != 0)
	{
		printf("e-Paper init failed\n");
		while(1);
	}

	Paint paint;
	Paint_Init(&paint, frame_buffer, epd.width, epd.height);
	Paint_Clear(&paint, UNCOLORED);

	/* Draw something to the frame_buffer */
	/* For simplicity, the arguments are explicit numerical coordinates */
	Paint_DrawRectangle(&paint, 20, 80, 180, 280, COLORED);
	Paint_DrawLine(&paint, 20, 80, 180, 280, COLORED);
	Paint_DrawLine(&paint, 180, 80, 20, 280, COLORED);
	Paint_DrawFilledRectangle(&paint, 200, 80, 360, 280, COLORED);
	Paint_DrawCircle(&paint, 300, 160, 60, UNCOLORED);
	Paint_DrawFilledCircle(&paint, 90, 210, 30, COLORED);

	/*Write strings to the buffer */
	Paint_DrawFilledRectangle(&paint, 0, 6, 400, 30, COLORED);
	Paint_DrawStringAt(&paint, 100, 10, "Hello world!", &Font24, UNCOLORED);
	Paint_DrawStringAt(&paint, 100, 40, "e-Paper Demo", &Font24, COLORED);

	while(1)
	{
		/* Display the frame_buffer */
		EPD_DisplayFrame(&epd, frame_buffer);

		HAL_Delay(50);
		/* Display the image buffer */
		EPD_DisplayFrame(&epd, IMAGE_BUTTERFLY);
		
		HAL_Delay(50);
	}
}



#endif






