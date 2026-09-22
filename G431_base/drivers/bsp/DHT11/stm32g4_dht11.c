/**
 *******************************************************************************
 * @file	stm32g4_dht11.c
 * @author	vchav
 * @date	Jun 12, 2024
 * @brief	Module pour utiliser le DHT11 (adaptation du module créé pour f103)
 *******************************************************************************
 */

#include "config.h"
#if USE_DHT11

#include "stm32g4_dht11.h"
#include "stm32g4_systick.h"
#include "stm32g4_gpio.h"
#include "stm32g4_extit.h"
#include <stdio.h>

#define NB_BITS	41	//le bit de poids fort n'appartiennent pas aux données utiles. (il s'agit de la réponse du capteur avant la trame utile).


static bool compute_frame(uint64_t datas, uint8_t * humidity_int, uint8_t * humidity_dec, uint8_t * temperature_int, uint8_t * temperature_dec);
static void DHT11_callback_exti(uint16_t pin);

static GPIO_TypeDef * DHT11_gpio;
static uint16_t DHT11_pin;
static bool initialized = false;
static uint32_t rising_time_us = 0;
static volatile bool flag_end_of_reception = false;
static volatile uint64_t trame;
static volatile uint8_t index = 0;
static volatile uint32_t t = 0;

/**
 * @brief Fonction pour initialiser les GPIO.
 * @param GPIOx: GPIO sur lequel est branché la broche data du capteur
 * @param GPIO_PIN_x: numéro du pin sur lequel est branché la broche data du capteur
 */
void BSP_DHT11_init(GPIO_TypeDef * GPIOx, uint16_t GPIO_PIN_x)
{
	DHT11_gpio = GPIOx;
	DHT11_pin = GPIO_PIN_x;
	HAL_GPIO_WritePin(DHT11_gpio, DHT11_pin, 1);
	BSP_GPIO_pin_config(DHT11_gpio, DHT11_pin, GPIO_MODE_OUTPUT_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
//	BSP_GPIO_pin_config(GPIOB, GPIO_PIN_3, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);	//utile à des fins de débogage
	initialized = true;
}

/**
 * @brief fonction de demo pour prendre en main les fonctions du module
 */
void BSP_DHT11_demo(void)
{
	static uint8_t humidity_int;
	static uint8_t humidity_dec;
	static uint8_t temperature_int;
	static uint8_t temperature_dec;

	BSP_DHT11_init(GPIOB, GPIO_PIN_0);
	while(1)
	{

		switch(BSP_DHT11_state_machine_get_datas(&humidity_int, &humidity_dec, &temperature_int, &temperature_dec))
		{
			case END_OK:
 				debug_printf("DHT11 h=%d,%d | t=%d,%d\n",humidity_int, humidity_dec, temperature_int, temperature_dec);
 				HAL_Delay(1500);
				break;
			case END_ERROR:
				debug_printf("DHT11 read error (h=%d,%d | t=%d,%d)\n", humidity_int, humidity_dec, temperature_int, temperature_dec);
				HAL_Delay(1500);
				break;
			case END_TIMEOUT:
				debug_printf("DHT11 timeout (h=%d,%d | t=%d,%d)\n", humidity_int, humidity_dec, temperature_int, temperature_dec);
				HAL_Delay(1500);
				break;
			default:
				break;
		}
	}
}


static void DHT11_process_ms(void)
{
	if(t)
		t--;
}

/**
 * @brief Cette fonction va être appelée automatiquement lorsqu'une donnée sera reçue sur le pin data du capteur.
 * @param pin: voir le fichier stm32g4_extit.c
 */
static void DHT11_callback_exti(uint16_t pin)
{
	if(1<<pin == DHT11_pin)
	{
		uint32_t current_time;
		current_time = BSP_systick_get_time_us();

		bool pin_state = HAL_GPIO_ReadPin(DHT11_gpio, DHT11_pin);
		if(index < NB_BITS)
		{
			if(pin_state)
			{
				rising_time_us = current_time;				//on enregistre la date du front montant (en microsecondes)
			}
			else if(rising_time_us)	//afin d'éviter le premier front descendant qui suit le lâcher du bus par le microcontrôleur.
			{						//on ne considère le front descendant que si on a vu le front montant qui le précède.
				uint32_t falling_time_us;
				falling_time_us = current_time; //on conserve la différence entre le front montant et le front descendant

				if(falling_time_us < rising_time_us)
				{
					falling_time_us+=1000;
				}
				if(falling_time_us - rising_time_us > 50)
				{
					trame |= (uint64_t)(1) << (NB_BITS - 1 - index);

				}
				index++;
			}
			//ce code permet de visualiser sur une sortie les instants d'exécution de cette IT.
	/*		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);
			Delay_us(1);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 0);
*/
		}

		if(index == NB_BITS)
		{
			flag_end_of_reception = true;
			BSP_EXTIT_disable(BSP_EXTIT_gpiopin_to_pin_number(DHT11_pin));
		}
	}
}


/**
 * @brief Machine d'état pour obtenir les données du capteur DHT11
 * @param humidity_int: Pointeur pour stocker la partie entière de l'humidité
 * @param humidity_dec: Pointeur pour stocker la partie décimale de l'humidité
 * @param temperature_int: Pointeur pour stocker la partie entière de la température
 * @param temperature_dec: Pointeur pour stocker la partie décimale de la température
 * @return L'état du processus (in progress, timeout, error, ou ok)
 * @note /!\ ATTENTION cette fonction est blocante pendant 4ms !
 */
running_t BSP_DHT11_state_machine_get_datas(uint8_t * humidity_int, uint8_t * humidity_dec, uint8_t * temperature_int, uint8_t * temperature_dec)
{
	typedef enum
	{
		INIT,
		SEND_START_SIGNAL,
		WAIT_DHT_ANSWER,
		TIMEOUT,
		END_OF_RECEPTION,
		WAIT_BEFORE_NEXT_ASK
	}state_e;
	static state_e state = INIT;
	static state_e previous_state = INIT;
	bool entrance;
	entrance = (state != previous_state)?true:false;
	previous_state = state;
	running_t ret = IN_PROGRESS;


	switch(state)
	{
		case INIT:
			if(initialized)
			{
				state = SEND_START_SIGNAL;
				BSP_systick_add_callback_function(&DHT11_process_ms);
				BSP_EXTIT_set_callback(&DHT11_callback_exti, BSP_EXTIT_gpiopin_to_pin_number(DHT11_pin), false);
			}
			else
			{
				debug_printf("You should call DHT11_init(...)\n");
				ret = END_ERROR;
			}
			break;

		case SEND_START_SIGNAL:
			if(entrance)
			{
				t = 20;
				index = 0;
				trame = 0;
				rising_time_us = 0;
				flag_end_of_reception = false;
				BSP_GPIO_pin_config(DHT11_gpio, DHT11_pin, GPIO_MODE_OUTPUT_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
				HAL_GPIO_WritePin(DHT11_gpio, DHT11_pin, 0);
			}
			if(!t)
			{
				BSP_GPIO_pin_config(DHT11_gpio, DHT11_pin, GPIO_MODE_IT_RISING_FALLING, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
				HAL_GPIO_WritePin(DHT11_gpio, DHT11_pin, 1);
				BSP_EXTIT_ack_it(BSP_EXTIT_gpiopin_to_pin_number(DHT11_pin));
				BSP_EXTIT_enable(BSP_EXTIT_gpiopin_to_pin_number(DHT11_pin));
				state = WAIT_DHT_ANSWER;
				//début de la surveillance des fronts
			}
			break;
		case WAIT_DHT_ANSWER:
			if(entrance)
			{
				t = 100;
			}
			if(flag_end_of_reception)
				state = END_OF_RECEPTION;
			if(!t)
			{
			//	UART_putc(UART1_ID, index);
				BSP_EXTIT_disable(BSP_EXTIT_gpiopin_to_pin_number(DHT11_pin));
				state = TIMEOUT;
			}
			break;
		case TIMEOUT:
			ret = END_TIMEOUT;
			t = 100;
			state = WAIT_BEFORE_NEXT_ASK;
			break;
		case END_OF_RECEPTION:
			printf("%llx\n",trame);
			if(compute_frame(trame, humidity_int, humidity_dec, temperature_int, temperature_dec))
				ret = END_OK;
			else
				ret = END_ERROR;
			t = 1000;
			state = WAIT_BEFORE_NEXT_ASK;
			break;
		case WAIT_BEFORE_NEXT_ASK:
			if(!t)
				state = SEND_START_SIGNAL;
			break;
		default:
			break;

	}
	return ret;
}

/**
 * @brief  Calcule et vérifie le checksum des données du capteur DHT11.
 * @param  datas: Données brutes du capteur DHT11.
 * @param  humidity_int: Pointeur pour stocker la partie entière de l'humidité.
 * @param  humidity_dec: Pointeur pour stocker la partie décimale de l'humidité.
 * @param  temperature_int: Pointeur pour stocker la partie entière de la température.
 * @param  temperature_dec: Pointeur pour stocker la partie décimale de la température.
 * @return true si le calcul et la vérification sont corrects, false sinon.
 */
static bool compute_frame(uint64_t datas, uint8_t * humidity_int, uint8_t * humidity_dec, uint8_t * temperature_int, uint8_t * temperature_dec)
{
	bool ret = false;
	*humidity_int = (uint8_t)(datas >> 32);
	*humidity_dec = (uint8_t)(datas >> 24);
	*temperature_int = (uint8_t)(datas >> 16);
	*temperature_dec = (uint8_t)(datas >> 8);
	printf(" - %d+%d+%d+%d = %d (%d) ", *humidity_int, *humidity_dec, *temperature_int, *temperature_dec, (uint8_t)(*humidity_int + *humidity_dec + *temperature_int + *temperature_dec), (uint8_t)(datas));
	//checksum
	if((uint8_t)(*humidity_int + *humidity_dec + *temperature_int + *temperature_dec) == (uint8_t)(datas))
		ret = true;
	return ret;
}























































#endif
