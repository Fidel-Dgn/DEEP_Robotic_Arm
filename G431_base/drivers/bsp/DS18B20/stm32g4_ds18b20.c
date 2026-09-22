/**
 *******************************************************************************
 * @file	stm32g4_ds18b20.c
 * @author	Nirgal 		&& Luc Hérault
 * @date	3 mai 2021 	&& Juin 2024 --> portage sur g431
 * @brief	Module pour utiliser le DS18b20
 *******************************************************************************
 */

#include "config.h"
#if USE_DS18B20
#include "stm32g4xx.h"
#include "stm32g4_systick.h"
#include "stm32g4_gpio.h"
#include "stm32g4_ds18b20.h"
#include "stdio.h"

#ifndef DS18B20_PIN
	#define DS18B20_PIN	GPIOB, GPIO_PIN_4
#endif

#define DS18B20_delay_us		Delay_us
#define DS18B20_delay_ms		HAL_Delay
#define DS18B20_pin_write(x)	HAL_GPIO_WritePin(DS18B20_PIN, x);
#define DS18B20_pin_read()		HAL_GPIO_ReadPin(DS18B20_PIN)

static volatile bool initialized = false;


static void DS18B20_Write (uint8_t data);

static uint8_t DS18B20_Read (void);

void BSP_DS18B20_init(void)
{
	if(!initialized)
	{
		BSP_GPIO_pin_config(DS18B20_PIN, GPIO_MODE_OUTPUT_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);  // set the pin as output opendrain
		DS18B20_pin_write(1);
		initialized = true;
	}
}

void BSP_DS18B20_demo(void)
{
	BSP_DS18B20_init();
	while(1)
	{
	   int16_t temperature;
	   temperature = BSP_DS18B20_get_temperature();
	   printf("temperature : %d\n", (temperature/16));
	   HAL_Delay(1000);
	}

}

int16_t BSP_DS18B20_get_temperature(void)
{
	uint8_t msb, lsb;
	msb = 0;
	lsb = 0;
	uint8_t presence;

	if(!initialized)
		BSP_DS18B20_init();

	presence = BSP_DS18B20_Start();
	if(presence)
	{
		DS18B20_delay_us(1000);
		DS18B20_Write (0xCC);  // skip ROM
		DS18B20_Write (0x44);  // convert t
		DS18B20_delay_ms(800);

		presence = BSP_DS18B20_Start();
		if(presence)
		{
			DS18B20_delay_us(1000);
			DS18B20_Write (0xCC);  // skip ROM
			DS18B20_Write (0xBE);  // Read Scratch-pad


			lsb = DS18B20_Read();
			msb = DS18B20_Read();
		}
	}

	return U16FROMU8(msb, lsb);
}

uint8_t BSP_DS18B20_Start (void)
{
	uint8_t response = 0;
	DS18B20_pin_write(0);	 // pull the pin low
	DS18B20_delay_us(480);   // delay according to datasheet

	DS18B20_pin_write(1);
	DS18B20_delay_us(80);    // delay according to datasheet

	if (!(DS18B20_pin_read()))
		response = 1;    // if the pin is low i.e the presence pulse is detected

	DS18B20_delay_us(400); // 480 us delay totally.

	return response;
}

static void DS18B20_Write (uint8_t data)
{
	for(int i=0; i<8; i++)
	{
		if((data & (1<<i))!=0)  // if the bit is high
		{	// write 1
			DS18B20_pin_write(0);  // pull the pin LOW
			DS18B20_delay_us(2);  // wait for 1 us

			DS18B20_pin_write(1);
			DS18B20_delay_us(60);  // wait for 60 us
		}

		else  // if the bit is low
		{	// write 0

			DS18B20_pin_write(0);
			DS18B20_delay_us(60);  // wait for 60 us

			DS18B20_pin_write(1);
			DS18B20_delay_us(15);	//wait for pull up !
		}
	}
}

static uint8_t DS18B20_Read (void)
{
	uint8_t value=0;

	for(int i=0;i<8;i++)
	{
		DS18B20_pin_write(0);  // pull the data pin LOW
		DS18B20_delay_us(2);  // wait for 2 us

		DS18B20_pin_write(1); // set as input
		DS18B20_delay_us(10);  // wait for pullup if the sensor do not write 0
		if(DS18B20_pin_read())  // if the pin is HIGH
		{
			value |= 1<<i;  // read = 1
		}
		DS18B20_delay_us(50);  // wait for the remaining 50 us (50+10 = 60)
	}
	return value;
}

#endif
