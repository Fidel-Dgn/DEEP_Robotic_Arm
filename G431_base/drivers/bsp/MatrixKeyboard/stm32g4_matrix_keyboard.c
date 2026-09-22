/**
 *******************************************************************************
 * @file	stm32g4_matrix_keyboard.c
 * @author	vchav
 * @date	Jun 10, 2024
 * @brief	Module pour utiliser un clavier matriciel (adaptation du module de Samuel Poiraud sur f103)
 *******************************************************************************
 */

#include "config.h"
#if USE_MATRIX_KEYBOARD
#include "stm32g4_matrix_keyboard.h"
#include "MCP23S17/stm32g4_mcp23s17.h"
#include "stm32g4_systick.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef CONFIG_PULL_UP
	#define DEFAULT_STATE 		(true)
	#define CONFIG_PULL	GPIO_PULLUP
#else
	#ifdef CONFIG_PULL_DOWN
		#define DEFAULT_STATE 	(false)
		#define CONFIG_PULL	GPIO_PULLDOWN
	#else
		#error "Vous devez définir CONFIG_PULL_UP ou CONFIG_PULL_DOWN"
	#endif
#endif


//Portage...
static void keyboard_pin_set_output(uint32_t port, uint16_t pin);
static void keyboard_pin_set_input(uint32_t port, uint16_t pin);
static void keyboard_pin_write(uint32_t port, uint16_t pin, bool state);
static bool keyboard_pin_read(uint32_t port, uint16_t pin);



//Fonctions privées
static char MATRIX_KEYBOARD_touch_to_key(uint32_t touchs_pressed);
static uint32_t MATRIX_KEYBOARD_read_all_touchs(void);
static uint8_t  MATRIX_KEYBOARD_get_inputs(void);
char MATRIX_KEYBOARD_get_key(void);
static void MATRIX_KEYBOARD_write_bit_output(uint8_t bit);

//Disposition des touches sur le clavier. (attention, ne correspond pas forcément à la disposition physique dans le bon ordre !)
const char default_keyboard_keys[16] = {
								'D','#','0','*',
								'C','9','8','7',
								'B','6','5','4',
								'A','3','2','1' };

char * keyboard_keys;
static bool initialized = false;

/* 	Autres propositions: Les positions des touches sur ces claviers sont probablement fausses...
	mais elles permettent de montrer l'utilisation de la fonction KEYBOARD_init(const char * new_keyboard_keys);
*/
const char custom_keyboard[16] =  {
										'0','4','8','C',
										'1','5','9','D',
										'2','6','A','E',
										'3','7','B','F' };
const char custom_keyboard_12_touchs[16]  =  {
										'1','2','3','X',
										'4','5','6','X',
										'7','8','9','X',
										'*','0','#','X' };

static volatile uint32_t t = 0;


/**
@brief	Initialise le module keyboard. Les ports concernés sont configurés en entrée.
*/
void BSP_MATRIX_KEYBOARD_init(const char * new_keyboard_keys)
{
	MATRIX_KEYBOARD_HAL_CLOCK_ENABLE();

	if((uint32_t)PORT_INPUT_0 <= MCP23S17_PORT_B || (uint32_t)PORT_INPUT_1 <= MCP23S17_PORT_B || (uint32_t)PORT_INPUT_2 <= MCP23S17_PORT_B || (uint32_t)PORT_INPUT_3 <= MCP23S17_PORT_B)
		BSP_MCP23S17_init();

	keyboard_pin_set_input((uint32_t)PORT_INPUT_0,PIN_INPUT_0);
	keyboard_pin_set_input((uint32_t)PORT_INPUT_1,PIN_INPUT_1);
	keyboard_pin_set_input((uint32_t)PORT_INPUT_2,PIN_INPUT_2);
	keyboard_pin_set_input((uint32_t)PORT_INPUT_3,PIN_INPUT_3);

	MATRIX_KEYBOARD_write_bit_output(DEFAULT_STATE);
	if(new_keyboard_keys)
		keyboard_keys = (char *)new_keyboard_keys;
	else
		keyboard_keys = (char *)default_keyboard_keys;
	initialized = true;
}


/**
 * @brief 	Cette fonction présente de façon simple l'utilisation de ce module logiciel.
 * @note	Cette fonction doit être appelée dans la boucle de tâche de fond.
 */
void BSP_MATRIX_KEYBOARD_demo_process_main (void)
{
	typedef enum
	{
		INIT = 0,
		RUN
	}state_e;
	static state_e state = INIT;

	char press_key_event, release_key_event;
	uint32_t all_touch_pressed;

	switch(state)
	{
		case INIT:
			BSP_systick_add_callback_function(BSP_MATRIX_KEYBOARD_demo_process_1ms);

			//A modifier en fonction du clavier utilisé : par défaut, personnalisé ou personnalisé 12 touches
			//KEYBOARD_init(NULL);						//Initialisation du clavier avec le clavier par défaut
			BSP_MATRIX_KEYBOARD_init(default_keyboard_keys);			//Initialisation du clavier avec un clavier personnalisé
			//KEYBOARD_init(custom_keyboard_12_touchs);	//Initialisation du clavier avec un clavier personnalisé 12 touches

			//pensez à renseigner les bons ports dans matrix_keyboard.h en fonction de votre hardware.
			printf("To run this demo, you should plug a matrix keyboard on the right ports. See matrix_keyboard.h\n");
			state = RUN;
			break;
		case RUN:

			//pour éviter les rebonds, il est important de lire le clavier toutes les 10ms environ.
			if(!t)	//A chaque fois que t vaut 0 (toutes les 10ms)...
			{
				t = 10;							//[ms] On recharge le chronomètre t pour 10ms...
				BSP_MATRIX_KEYBOARD_press_and_release_events(&press_key_event, &release_key_event, &all_touch_pressed);
				switch(press_key_event)
				{
					case NO_KEY:
						break;
					case MANY_KEYS:
						printf("Many keys pressed : %lx\n", all_touch_pressed);
						break;
					default:
						printf("%c pressed\n", press_key_event);
						break;
				}
				switch(release_key_event)
				{
					case NO_KEY:
						break;
					case MANY_KEYS:
						printf("Many keys released : %lx\n", all_touch_pressed);
						break;
					default:
						printf("%c released\n", release_key_event);
						break;
				}
			}
			break;
		default:
			break;
	}
}


//Cette fonction doit être appelée toutes les ms.
void BSP_MATRIX_KEYBOARD_demo_process_1ms(void)
{
	if(t)		//Si le chronomètre est "chargé", on décompte... (comme un minuteur de cuisine !)
		t--;
}


/**
@brief	Vérifie qu'il y a appui sur une touche ou non.
@return	true si une touche est appuyée, false sinon.
*/
bool BSP_MATRIX_KEYBOARD_is_pressed(void)
{
	uint8_t ret;
	if(!initialized)
		return false;

	MATRIX_KEYBOARD_write_bit_output(!DEFAULT_STATE);

	//Si l'un des ports n'est pas dans l'état par défaut, c'est qu'une touche est pressée.
	ret = (	(keyboard_pin_read((uint32_t)PORT_INPUT_0,PIN_INPUT_0) != DEFAULT_STATE) ||
			(keyboard_pin_read((uint32_t)PORT_INPUT_1,PIN_INPUT_1) != DEFAULT_STATE) ||
			(keyboard_pin_read((uint32_t)PORT_INPUT_2,PIN_INPUT_2) != DEFAULT_STATE) ||
			(keyboard_pin_read((uint32_t)PORT_INPUT_3,PIN_INPUT_3) != DEFAULT_STATE));

	MATRIX_KEYBOARD_write_bit_output(DEFAULT_STATE);

	return ret;
}


/**
 * @brief	Cette fonction met à jour press_event, release_event et all_touchs_pressed avec les informations reçues et traitées.
 * 			Il est recommandé d'appeler cette fonction toutes les 10ms. (à des fins d'anti rebond logiciel, et pour ne louper aucun évènement).
 * @param 	press_event: va prendre soit la valeur NO_KEY, soit MANY_KEYS, soit le caractère appuyé sur le clavier (voir fonction MATRIX_KEYBOARD_touch_to_key())
 * @param 	release_event: va prendre soit la valeur NO_KEY, soit MANY_KEYS, soit le caractère relaché sur le clavier (voir fonction MATRIX_KEYBOARD_touch_to_key())
 * @param 	all_touchs_pressed: Permet de récupérer l'état de l'ensemble du clavier (1 bit par touche)
 * @pre 	KEYBOARD_init() doit avoir été appelée avant.
 * @post	/!\ si deux évènement d'appuis au moins sont simultanés, press_event reçoit l'information MANY_KEYS
 * 			si deux évènement de relachement au moins sont simultanés, release_event reçoit l'information MANY_KEYS /!\
 */
void BSP_MATRIX_KEYBOARD_press_and_release_events(char * press_event, char * release_event, uint32_t * all_touchs_pressed)
{
	static uint32_t previous_touchs = 0;
	uint32_t current_touchs, up_touchs, down_touchs;

	if(!initialized)
		return;

	current_touchs = MATRIX_KEYBOARD_read_all_touchs();
	down_touchs = ~current_touchs & previous_touchs;
	up_touchs = current_touchs & ~previous_touchs;
	previous_touchs = current_touchs;

	*all_touchs_pressed = current_touchs;
	*press_event = MATRIX_KEYBOARD_touch_to_key(up_touchs);
	*release_event = MATRIX_KEYBOARD_touch_to_key(down_touchs);
}

/**
@brief	Renvoi le code ASCII de la touche pressée. En correspondance avec le tableau de codes ASCII.
@post	Cette fonction intègre un anti-rebond  TODO A bon ?
@pre	Il est conseillé d'appeler cette fonction périodiquement (10ms par exemple)
@return	Retourne le caractère ASCII si UNE touche est pressée. Sinon, renvoie 0.
*/
char MATRIX_KEYBOARD_get_key(void)
{
	if(!initialized)
		return false;
	uint32_t touchs_pressed;
	touchs_pressed = MATRIX_KEYBOARD_read_all_touchs();
	return MATRIX_KEYBOARD_touch_to_key(touchs_pressed);
}

/**
 * @brief Regarde si plusieur, 0 ou 1 seul touche(s) à/ont été touchée(s). Si 1 seul touche: elle renvoit la lettre ascii correspondante sur le clavier.
 * @param touchs_pressed: un chiffre qui, une fois écrit en binaire, indique quelle(s) touche(s) à /ont été touchée(s) (ex: 0000000000010000 = keyboard_keys[4] = 'C')
 * @return soit NO_KEY, soit MANY_KEYS, soit le caractère appuyé ou relaché sur le clavier
 */
static char MATRIX_KEYBOARD_touch_to_key(uint32_t touchs_pressed)
{
	uint32_t index;
	if(touchs_pressed == 0)
		return NO_KEY;
	else if((touchs_pressed & (touchs_pressed-1)))
		return MANY_KEYS;
	else
	{
		//touchs_pressed ne contient qu'un seul bit à 1
		for(index = 0; index < 16; index++)
		{
			if(touchs_pressed == 1u<<index)
				return keyboard_keys[index];
		}
	}
	return MANY_KEYS;	//n'arrive jamais.
}


static uint32_t MATRIX_KEYBOARD_read_all_touchs(void)
{
	uint32_t ret;
	uint8_t i;
	ret = 0;

	for(i=0;i<4;i++)
	{
		MATRIX_KEYBOARD_write_bit_output(DEFAULT_STATE);
		switch(i)
		{
			case 0:
				keyboard_pin_set_output((uint32_t)PORT_OUTPUT_0,PIN_OUTPUT_0);
				keyboard_pin_write((uint32_t)PORT_OUTPUT_0,PIN_OUTPUT_0,!DEFAULT_STATE);
				break;
			case 1:
				keyboard_pin_set_output((uint32_t)PORT_OUTPUT_1,PIN_OUTPUT_1);
				keyboard_pin_write((uint32_t)PORT_OUTPUT_1,PIN_OUTPUT_1,!DEFAULT_STATE);
				break;
			case 2:
				keyboard_pin_set_output((uint32_t)PORT_OUTPUT_2,PIN_OUTPUT_2);
				keyboard_pin_write((uint32_t)PORT_OUTPUT_2,PIN_OUTPUT_2,!DEFAULT_STATE);
				break;
			case 3:
				keyboard_pin_set_output((uint32_t)PORT_OUTPUT_3,PIN_OUTPUT_3);
				keyboard_pin_write((uint32_t)PORT_OUTPUT_3,PIN_OUTPUT_3,!DEFAULT_STATE);
				break;
			default:
				break;	//Ne doit pas se produire.
		}
		//Acquisition entrées.
		ret |= (uint32_t)(MATRIX_KEYBOARD_get_inputs()) << (4*i);
	}
	//printf("t%lx\n",ret);
	return ret;
}

/**
@brief  Cette fonction lit l'état des entrées.
@return Retourne un entier contenant l'état des entrées.
*/
static uint8_t  MATRIX_KEYBOARD_get_inputs(void)
{
	uint32_t inputs;
	inputs = 	((keyboard_pin_read((uint32_t)PORT_INPUT_0,PIN_INPUT_0) != DEFAULT_STATE)?1u:0)|
				((keyboard_pin_read((uint32_t)PORT_INPUT_1,PIN_INPUT_1) != DEFAULT_STATE)?2u:0)|
				((keyboard_pin_read((uint32_t)PORT_INPUT_2,PIN_INPUT_2) != DEFAULT_STATE)?4u:0)|
				((keyboard_pin_read((uint32_t)PORT_INPUT_3,PIN_INPUT_3) != DEFAULT_STATE)?8u:0);
	return (uint8_t)inputs;
}


static void MATRIX_KEYBOARD_write_bit_output(uint8_t bit)
{
	if(bit == DEFAULT_STATE){
		keyboard_pin_set_input((uint32_t)PORT_OUTPUT_0,PIN_OUTPUT_0);
		keyboard_pin_set_input((uint32_t)PORT_OUTPUT_1,PIN_OUTPUT_1);
		keyboard_pin_set_input((uint32_t)PORT_OUTPUT_2,PIN_OUTPUT_2);
		keyboard_pin_set_input((uint32_t)PORT_OUTPUT_3,PIN_OUTPUT_3);
	}
	else{
		keyboard_pin_write((uint32_t)PORT_OUTPUT_0,PIN_OUTPUT_0,bit);
		keyboard_pin_write((uint32_t)PORT_OUTPUT_1,PIN_OUTPUT_1,bit);
		keyboard_pin_write((uint32_t)PORT_OUTPUT_2,PIN_OUTPUT_2,bit);
		keyboard_pin_write((uint32_t)PORT_OUTPUT_3,PIN_OUTPUT_3,bit);
		keyboard_pin_set_output((uint32_t)PORT_OUTPUT_0,PIN_OUTPUT_0);
		keyboard_pin_set_output((uint32_t)PORT_OUTPUT_1,PIN_OUTPUT_1);
		keyboard_pin_set_output((uint32_t)PORT_OUTPUT_2,PIN_OUTPUT_2);
		keyboard_pin_set_output((uint32_t)PORT_OUTPUT_3,PIN_OUTPUT_3);
	}
}

static bool mcp23s17_initialized = false;
static char * warning_string = "you should clarify the parameters according to the requirement of this keyboard driver !\n";

static void keyboard_pin_set_output(uint32_t port, uint16_t pin)
{
	if(port <= MCP23S17_PORT_B && pin && ((pin&(pin-1))==0))	//on v�rifie que pin est bien une puissance de 2.
	{
		if(!mcp23s17_initialized)
		{
			BSP_MCP23S17_init();
			mcp23s17_initialized = true;
		}
		BSP_MCP23S17_setGPIODirection((MCP23S17_port_e)port, pin, MCP23S17_DIRECTION_OUTPUT);
	}
	else if(port >= (uint32_t)GPIOA && port <= (uint32_t)GPIOG)
	{
		HAL_GPIO_Init((GPIO_TypeDef *)port, &(GPIO_InitTypeDef){pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH});
	}
	else
	{
		printf(warning_string);
	}
}

static void keyboard_pin_set_input(uint32_t port, uint16_t pin)
{
	if(port <= MCP23S17_PORT_B && pin && ((pin&(pin-1))==0))	//on v�rifie que pin est bien une puissance de 2.
	{
		if(!mcp23s17_initialized)
		{
			BSP_MCP23S17_init();
			mcp23s17_initialized = true;
		}
		BSP_MCP23S17_setGPIODirection((MCP23S17_port_e)port, pin, MCP23S17_DIRECTION_INPUT);
		BSP_MCP23S17_setPullUp_onPin((MCP23S17_port_e)port, pin, (CONFIG_PULL==GPIO_PULLUP)?MCP23S17_PULL_UP_STATE_HIGH:MCP23S17_PULL_UP_STATE_LOW);
	}
	else if (port >= (uint32_t)GPIOA && port <= (uint32_t)GPIOG)
	{
		HAL_GPIO_Init((GPIO_TypeDef*) port, &(GPIO_InitTypeDef ) { pin,	GPIO_MODE_INPUT, CONFIG_PULL, GPIO_SPEED_FREQ_HIGH });
	}
	else
	{
		printf(warning_string);
	}
}

static void keyboard_pin_write(uint32_t port, uint16_t pin, bool state)
{
	if(port <= MCP23S17_PORT_B && pin && ((pin&(pin-1))==0))	//on v�rifie que pin est bien une puissance de 2.
	{
		if(!mcp23s17_initialized)
		{
			BSP_MCP23S17_init();
			mcp23s17_initialized = true;
		}
		BSP_MCP23S17_writeGPIO((MCP23S17_port_e)port, pin, (MCP23S17_pinState_e)state);
	}
	else if (port >= (uint32_t)GPIOA && port <= (uint32_t)GPIOG)
	{
		HAL_GPIO_WritePin((GPIO_TypeDef*) port, pin, (GPIO_PinState)state);
	}
	else
	{
		printf(warning_string);
	}
}

static bool keyboard_pin_read(uint32_t port, uint16_t pin)
{
	bool ret = false;
	if(port <= MCP23S17_PORT_B && pin && ((pin&(pin-1))==0))	//on v�rifie que pin est bien une puissance de 2.
	{
		if(!mcp23s17_initialized)
		{
			BSP_MCP23S17_init();
			mcp23s17_initialized = true;
		}
		ret = (BSP_MCP23S17_readGPIO((MCP23S17_port_e)port)&pin)?true:false;
	}
	else if (port >= (uint32_t)GPIOA && port <= (uint32_t)GPIOG)
	{
		ret = HAL_GPIO_ReadPin((GPIO_TypeDef*) port, pin);
	}
	else
	{
		printf(warning_string);
	}
	return ret;
}



#endif
