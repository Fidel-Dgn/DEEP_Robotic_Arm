#include "config.h"
#include "stm32g4_utils.h"
#include "stm32g4xx.h"
#include "stm32g4_timer.h"
#include "stm32g4_stepper_motor.h"
#include "stm32g4_sys.h"
#include "stm32g4_gpio.h"

/*
Comment utiliser ce module logiciel ? (qui est encore en phase de développement... il faut donc bien le comprendre pour s'en servir !)
1- définir le nombre de moteurs dans le fichier header, en adaptant l'enumeration motor_id_e.
2- adapter ci-dessous les tableaux enable_pins, dir_pins, pulse_pins qui indiquent les broches de pilotage des moteurs pas à pas.

*/

#if USE_STEPPER_MOTOR


#ifndef STEPPER_MOTOR_TIMER
	#define STEPPER_MOTOR_TIMER					TIMER2_ID
	#define STEPPER_MOTOR_timer_irq_handler		TIMER2_user_handler_it
#endif

#ifndef STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_OPENDRAIN_OUTPUTS
	#define STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_OPENDRAIN_OUTPUTS 0
#endif
#ifndef STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_MOSFET
	#define STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_MOSFET 1
#endif

#if (STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_OPENDRAIN_OUTPUTS + STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_MOSFET != 1)
	#warning "vous devez choisir l'un de ces deux modes, RDV dans config.h !"
#endif

#if (STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_MOSFET == 1)
	#define SLEEPING_STATE	0			//avec des mosfets pour piloter les entrées "PUL-", "EN-", "DIR-" du TB6600, la logique est inversée.
#else
	#define SLEEPING_STATE	1			//avec une liaison directe pour piloter les entrées "PUL-", "EN-", "DIR-" du TB6600, un "1" bascule la sortie en opendrain, la led de l'optocoupleur n'est plus pilotée.
#endif


#define DEFAULT_IT_PERIOD 100			//période de l'interruption qui génère les pulses.



static GPIO_TypeDef * enable_gpios[STEPPER_MOTOR_NB] = 	{NULL, NULL};	//définir ici les numéros de broches des enables des moteurs  (sur le GPIO unique GPIO_STEPPER_MOTOR)
static GPIO_TypeDef * dir_gpios[STEPPER_MOTOR_NB] = 	{GPIOB, GPIOA};	//ici les broches des directions
static GPIO_TypeDef * pulse_gpios[STEPPER_MOTOR_NB] = 	{GPIOA, GPIOA};	//ici les broches des pulse

static const uint8_t enable_pins[STEPPER_MOTOR_NB] = 	{0, 0};	//définir ici les numéros de broches des enables des moteurs  (sur le GPIO unique GPIO_STEPPER_MOTOR)
static const uint8_t dir_pins[STEPPER_MOTOR_NB] = 		{14, 12};	//ici les broches des directions
static const uint8_t pulse_pins[STEPPER_MOTOR_NB] = 	{15, 13};	//ici les broches des pulse

static volatile int32_t positions[STEPPER_MOTOR_NB] = {0};
static volatile int32_t goals[STEPPER_MOTOR_NB] = {0};
static volatile uint32_t pulse_period[STEPPER_MOTOR_NB] = {10, 10};	//"vitesse" par défaut (période par défaut entre deux pulses)
static callback_fun_t callback_at_each_pulse = NULL;

static void STEPPER_MOTOR_pin_set(GPIO_TypeDef * gpio, uint32_t pin, bool b);


/////////////////////Fonctions publiques///////////////////////


void STEPPER_MOTOR_init(void)
{
	for(motor_id_e m=0; m<STEPPER_MOTOR_NB; m++)
	{
		uint32_t gpio_mode;
		#if STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_MOSFET
			gpio_mode = GPIO_MODE_OUTPUT_PP;
		#else
			gpio_mode = GPIO_MODE_OUTPUT_OD;
		#endif
		//Configurer les sorties
		if(enable_gpios[m] != NULL)
			BSP_GPIO_pin_config(enable_gpios[m], 	1<<enable_pins[m],	gpio_mode,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
		BSP_GPIO_pin_config(dir_gpios[m], 		1<<dir_pins[m], 	gpio_mode,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);
		BSP_GPIO_pin_config(pulse_gpios[m], 	1<<pulse_pins[m],	gpio_mode,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH, GPIO_NO_AF);

		//Appliquer l'�tat de repos sur toutes les sorties
		if(enable_gpios[m] != NULL)
			STEPPER_MOTOR_pin_set(enable_gpios[m], 	enable_pins[m], SLEEPING_STATE);
		STEPPER_MOTOR_pin_set(dir_gpios[m], 	dir_pins[m], SLEEPING_STATE);
		STEPPER_MOTOR_pin_set(pulse_gpios[m], 	pulse_pins[m], SLEEPING_STATE);
	
		STEPPER_MOTOR_enable(m, true);
	}
	
	BSP_TIMER_run_us(STEPPER_MOTOR_TIMER, DEFAULT_IT_PERIOD, true);
}

void STEPPER_MOTOR_demo(void)
{
	STEPPER_MOTOR_init();

	bool toggle0 = false;
	bool toggle1 = false;
	while(1)
	{
		if(STEPPER_MOTOR_is_arrived(STEPPER_MOTOR_0))
		{
			STEPPER_MOTOR_set_goal(STEPPER_MOTOR_0,(toggle0)?1000:-1000);
			toggle0 = !toggle0;
		}

		if(STEPPER_MOTOR_is_arrived(STEPPER_MOTOR_1))
		{
			STEPPER_MOTOR_set_goal(STEPPER_MOTOR_1,(toggle1)?900:-900);
			toggle1 = !toggle1;
		}
	}
}


void STEPPER_MOTOR_set_callback_at_each_pulse(callback_fun_t cb)
{
	callback_at_each_pulse = cb;
}

void STEPPER_MOTOR_enable(motor_id_e id, bool enable)
{
	STEPPER_MOTOR_pin_set(enable_gpios[id], enable_pins[id], (!enable)^SLEEPING_STATE);
}



void STEPPER_MOTOR_set_goal(motor_id_e id, int32_t newgoal)
{
	if(id<STEPPER_MOTOR_NB)
		goals[id] = newgoal;

}

int32_t STEPPER_MOTOR_get_goal(motor_id_e id)
{
	if(id<STEPPER_MOTOR_NB)
		return goals[id];
	return 0;
}

int32_t STEPPER_MOTOR_get_position(motor_id_e id){
	return positions[id];
}

void STEPPER_MOTOR_set_position(motor_id_e id, int32_t newposition)
{
	if(id<STEPPER_MOTOR_NB)
		positions[id] = newposition;
}

bool STEPPER_MOTOR_is_arrived (motor_id_e id){
	return (goals[id]==positions[id])?true:false;
}



/////////////////////Fonctions priv�es///////////////////////


static void STEPPER_MOTORS_do_pulse(motor_id_e id)
{
	STEPPER_MOTOR_pin_set(pulse_gpios[id], pulse_pins[id], !SLEEPING_STATE);
	Delay_us(50);
	STEPPER_MOTOR_pin_set(pulse_gpios[id], pulse_pins[id], SLEEPING_STATE);
	if(callback_at_each_pulse != NULL)
		callback_at_each_pulse();
}

static void STEPPER_MOTORS_set_dir(motor_id_e id, bool direction)
{
	STEPPER_MOTOR_pin_set(dir_gpios[id], dir_pins[id], direction^SLEEPING_STATE);
}


static void STEPPER_MOTOR_pin_set(GPIO_TypeDef * gpio, uint32_t pin, bool b)
{
	if(gpio == NULL)
		return;
	#if STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_OPENDRAIN_OUTPUTS
		if(pin<8)
		{
			if(b)
				GPIO_STEPPER_MOTOR->CRL &= (uint32_t)(~(3<<(4*(pin))));
			else
				GPIO_STEPPER_MOTOR->CRL |= (3<<(4*(pin)));
		}
		else
		{
			if(b)
				GPIO_STEPPER_MOTOR->CRH &= (uint32_t)(~(3<<(4*(pin - 8))));
			else
				GPIO_STEPPER_MOTOR->CRH |= (3<<(4*(pin - 8)));
		}
	#endif
	#if STEPPER_DRIVER_TB6600_IS_DRIVEN_BY_MOSFET
		if(b)
			gpio->BSRR = (1<<pin);
		else
			gpio->BSRR = (uint32_t)(1<<pin<<16);
	#endif

}




void STEPPER_MOTOR_timer_irq_handler (){
	static uint32_t periods[STEPPER_MOTOR_NB] = {0};

	for(motor_id_e m=0; m<STEPPER_MOTOR_NB; m++)
	{
		periods[m] = (uint32_t)((uint32_t)(periods[m]+1)%pulse_period[m]);
		if (periods[m] == 0)
		{
			if (positions[m] < goals[m])
			{
					positions[m]++;
					STEPPER_MOTORS_set_dir(m, false);
					STEPPER_MOTORS_do_pulse(m);
			}
			else if (positions[m] > goals[m])
			{
					positions[m]--;
					STEPPER_MOTORS_set_dir(m, true);
					STEPPER_MOTORS_do_pulse(m);
			}
		}
	}
}


#endif
