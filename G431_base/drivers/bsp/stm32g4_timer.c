/**
 *******************************************************************************
 * @file	stm32g4_timer.c
 * @author	jjo
 * @date	Apr 26, 2024
 * @brief	Module de gestion des timers pour cible STM32G4
 * 			Adaptation du module stm32f1_timer.c de S.Poiraud
 *******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/
#include "stm32g4_timer.h"
#include "stm32g4xx_hal_tim.h"
#include "stm32g4_sys.h"
#include "stm32g4_gpio.h"

#if USE_BSP_TIMER | 1

/* Private defines -----------------------------------------------------------*/
/**
 * 	@brief Macro pour obtenir la valeur maximale que peut prendre la période d'un timer
 *
 * Timer 2 est un timer 32 bits, les autres sont des timers 16 bits.
 */
#define GET_MAX_PERIOD(timer_id) (timer_id == TIMER2_ID ? UINT32_MAX : UINT16_MAX)

/* Private types -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef structure_handles[TIMER_ID_NB]; //Ce tableau contient les structures qui sont utilisées pour piloter chaque TIMER avec la librairie HAL.

/* Private constants ---------------------------------------------------------*/
static const TIM_TypeDef * instance_array[TIMER_ID_NB] = {TIM1, TIM2, TIM3, TIM4, TIM6};
static const IRQn_Type nvic_irq_array[TIMER_ID_NB] = {TIM1_UP_TIM16_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM6_DAC_IRQn};


/* Private functions declarations --------------------------------------------*/


/* Private functions definitions ---------------------------------------------*/
/**
 * @brief	Acquitte les IT sur le timer sélectionné.
 * @pre 	Le timer a ete initialisé
 * @post	L'interruption est acquitée
 */
void clear_it_status(timer_id_t timer_id){
	switch(timer_id)
	{
		case TIMER1_ID:
			__HAL_TIM_CLEAR_IT(&structure_handles[TIMER1_ID], TIM_IT_UPDATE);
			break;
		case TIMER2_ID:
			__HAL_TIM_CLEAR_IT(&structure_handles[TIMER2_ID], TIM_IT_UPDATE);
			break;
		case TIMER3_ID:
			__HAL_TIM_CLEAR_IT(&structure_handles[TIMER3_ID], TIM_IT_UPDATE);
			break;
		case TIMER4_ID:
			__HAL_TIM_CLEAR_IT(&structure_handles[TIMER4_ID], TIM_IT_UPDATE);
			break;
		case TIMER6_ID:
			__HAL_TIM_CLEAR_IT(&structure_handles[TIMER6_ID], TIM_IT_UPDATE);
		default:
			break;
	}
}

/* Public functions definitions ----------------------------------------------*/

/**
 * @brief Initialisation et lancement du timer sélectionné.
 *
 * Cette fonction lance le timer et le configure. Elle peut les IT quand il y a débordement du timer.
 * @pre Les interruptions du timer sont désactivées, par exemple via BSP_TIMER_stop() (risque de déclenchement intempestif pendant la configuration)
 * @param id du timer	cf timer_id_t
 * @param us 			temps en us codé sur un 32bits non signé
 * @param enable_irq	TRUE : active les IT, FALSE : ne les active pas. En cas d'activation des IT, l'utilisateur doit écrire une fonction TIMERx_user_handler_it. Par défaut, ces fonctions écrites dans ce fichier mais avec l'attribut weak (elles peuvent donc être réécrites)
 * @post Le timer et son horloge sont activés, ses interruptions autorisées (si activées), et son décompte lancé.
 */
void BSP_TIMER_run_us(timer_id_t timer_id, uint32_t us, bool enable_irq)
{
	// On active l'horloge du timer concerné.
	switch(timer_id)
	{
		case TIMER1_ID:
			__HAL_RCC_TIM1_CLK_ENABLE();
			break;
		case TIMER2_ID:
			__HAL_RCC_TIM2_CLK_ENABLE();
			break;
		case TIMER3_ID:
			__HAL_RCC_TIM3_CLK_ENABLE();
			break;
		case TIMER4_ID:
			__HAL_RCC_TIM4_CLK_ENABLE();
			break;
		case TIMER6_ID:
			__HAL_RCC_TIM6_CLK_ENABLE();
		default:
			break;
	}

	// Time base configuration
	structure_handles[timer_id].Instance = (TIM_TypeDef*)instance_array[timer_id]; //On donne le timer en instance à notre gestionnaire (Handle)

	//On détermine la fréquence des évènements comptés par le timer.
	uint32_t freq;
	if(timer_id == TIMER1_ID)
	{
		//Fréquence du TIMER1 est PCLK2 lorsque APB2 Prescaler vaut 1, sinon : PCLK2*2
		freq = HAL_RCC_GetPCLK2Freq();
		if((RCC->CFGR & RCC_CFGR_PPRE2) >> 11 != RCC_HCLK_DIV1)
			freq *= 2;
	}
	else
	{
		//Fréquence des TIMERS 2,3,4 est PCLK1 lorsque APB1 Prescaler vaut 1, sinon : PCLK1*2
		freq = HAL_RCC_GetPCLK1Freq();
		if((RCC->CFGR & RCC_CFGR_PPRE1) >> 8 != RCC_HCLK_DIV1)
			freq *= 2;
	}

	uint64_t nb_psec_per_event = (uint64_t)(1000000000000/freq);
	uint64_t period = (((uint64_t)(us))*1000000)/nb_psec_per_event;

	uint32_t max_period = GET_MAX_PERIOD(timer_id);

	if(period > max_period)
	{
		uint32_t prescaler = 1;
		while(period > max_period)
		{
			prescaler *= 2;
			period /= 2;
		}
		structure_handles[timer_id].Init.Prescaler 	= prescaler - 1;	//le prescaler du timer doit être enregistré avec un offset de -1.
		structure_handles[timer_id].Init.Period 	= (uint32_t)(period - 1);	//On compte de 0 à period-1
	}
	else
	{
		structure_handles[timer_id].Init.Prescaler 	= 0;
		structure_handles[timer_id].Init.Period 	= (uint32_t)(period - 1);
	}

	structure_handles[timer_id].Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	structure_handles[timer_id].Init.CounterMode = TIM_COUNTERMODE_UP;

	// On applique les paramètres d'initialisation
	if (HAL_TIM_Base_Init(&structure_handles[timer_id]) != HAL_OK)
	{
		Error_Handler();
	}

	if(enable_irq)
	{
		//acquittement des IT
		clear_it_status(timer_id);
		// On fixe les priorités des interruptions du timer PreemptionPriority = 0, SubPriority = 1 et on autorise les interruptions
		HAL_NVIC_SetPriority(nvic_irq_array[timer_id] , 4,  1);
		HAL_NVIC_EnableIRQ(nvic_irq_array[timer_id]);
	}

	// On autorise les interruptions
	if(HAL_TIM_Base_Start_IT(&structure_handles[timer_id]) != HAL_OK)
    {
        Error_Handler();
    }

	// On lance le timer
	__HAL_TIM_ENABLE(&structure_handles[timer_id]);
}

void BSP_TIMER_enable_irq(timer_id_t timer_id)
{
	//acquittement des IT
	clear_it_status(timer_id);
	// On fixe les priorités des interruptions du timer PreemptionPriority = 0, SubPriority = 1 et on autorise les interruptions
	HAL_NVIC_SetPriority(nvic_irq_array[timer_id] , 4,  1);
	HAL_NVIC_EnableIRQ(nvic_irq_array[timer_id]);
}

void BSP_TIMER_disable_irq(timer_id_t timer_id)
{
	HAL_NVIC_DisableIRQ(nvic_irq_array[timer_id]);
}

/**
 * @brief Arrêt du timer sélectionné.
 *
 * Cette fonction arrête le timer.
 * @param id du timer	cf timer_id_t
 * @pre Le timer est initialisé.
 * @post Le timer et son horloge sont désactivés.
 */
void BSP_TIMER_stop(timer_id_t timer_id)
{
    // On désactive le timer et ses interruptions
	if(HAL_TIM_Base_Stop_IT(&structure_handles[timer_id]) != HAL_OK)
    {
        Error_Handler();
    }
}

void BSP_TIMER_start(timer_id_t timer_id)
{
	// On active le timer
	if(HAL_TIM_Base_Start_IT(&structure_handles[timer_id]) != HAL_OK)
	{
		Error_Handler();
	}
}

uint32_t BSP_TIMER_read(timer_id_t timer_id)
{
	return __HAL_TIM_GET_COUNTER(&structure_handles[timer_id]);
}

void BSP_TIMER_write(timer_id_t timer_id, uint32_t counter)
{
	if(timer_id == TIMER2_ID)	// TIMER2 est un timer 32 bits, les autres sont des timers 16 bits.
    {
        __HAL_TIM_SET_COUNTER(&structure_handles[timer_id], counter);
    }
    else
    {
        __HAL_TIM_SET_COUNTER(&structure_handles[timer_id], (uint16_t)counter);
    }
}


uint32_t BSP_TIMER_get_period(timer_id_t timer_id)
{
    return structure_handles[timer_id].Init.Period + 1;
}

void BSP_TIMER_set_period(timer_id_t timer_id, uint32_t period)
{
	if(timer_id == TIMER2_ID)	// TIMER2 est un timer 32 bits, les autres sont des timers 16 bits.
    {
        __HAL_TIM_SET_AUTORELOAD(&structure_handles[timer_id], period - 1);
    }
    else
    {
        __HAL_TIM_SET_AUTORELOAD(&structure_handles[timer_id], (uint16_t)period - 1);
    }
}

uint16_t BSP_TIMER_get_prescaler(timer_id_t timer_id)
{
    return structure_handles[timer_id].Init.Prescaler + 1;
}

void BSP_TIMER_set_prescaler(timer_id_t timer_id, uint16_t prescaler)
{
	__HAL_TIM_SET_PRESCALER(&structure_handles[timer_id], prescaler - 1);
}

void BSP_TIMER_enable_output_trigger(timer_id_t timer_id)
{
	TIM_MasterConfigTypeDef sMasterConfig;
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // TIM_TRGO_OC1
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	HAL_TIMEx_MasterConfigSynchronization(&structure_handles[timer_id], &sMasterConfig);
	HAL_TIM_GenerateEvent(&structure_handles[timer_id],TIM_EVENTSOURCE_UPDATE);
}
/**
 * @brief Accesseur du handler
 */
TIM_HandleTypeDef * BSP_TIMER_get_handler(timer_id_t timer_id)
{
    return &structure_handles[timer_id];
}

/**
 * @brief Fonction de configuration de la PWM
 *
 * Cette fonction configure un timer en mode PWM et active la PWM sur le canal sélectionné.
 * @param timer_id id du timer (cf timer_id_t)
 * @param TIM_CHANNEL_x canal de sortie
 * @param duty rapport cyclique de la PWM
 * @param remap permet de remapper arbitrairement sur le prochain numéro de broche disponible (pas de fonction par défaut sur la STM32G4)
 * 			TIM1_CH1N : PA11	(remap)PA7
 * 			TIM1_CH2N : PA12	(remap)PB0
 * 			TIM1_CH3N : PF0
 * 			TIM1_CH1  : PA8
 * 			TIM1_CH2  : PA9
 * 			TIM1_CH3  : PA10
 * 			TIM1_CH4  : PA11
 *
 * 			TIM2_CH1  : PA0		(remap)PA5		(non géré)PA15
 * 			TIM2_CH2  : PA1		(remap)PB3
 * 			TIM2_CH3  : PA2		(remap)PA9
 * 			TIM2_CH4  : PA3		(remap)PA10
 *
 * 			TIM3_CH1  : PA6		(remap)PB4
 * 			TIM3_CH2  : PA4		(remap)PA7		(non géré)PB5
 * 			TIM3_CH3  : PB0
 * 			TIM3_CH4  : PB7
 *
 * 			TIM4_CH1  : PA11	(rempa)PB6
 * 			TIM4_CH2  : PA12	(rempa)PB7
 * 			TIM4_CH3  : PA13	(remap)PB8
 * @param negative_channel TRUE: active le canal négatif, FALSE: désactive le canal négatif
 * @pre Le timer est initialisé.
 * @post Le timer est configuré en mode PWM et la PWM est activée sur le canal sélectionné.
 */
void BSP_TIMER_enable_PWM(timer_id_t timer_id, uint16_t TIM_CHANNEL_x, uint16_t duty, bool remap, bool negative_channel)
{
    switch(timer_id)
    {
    case TIMER1_ID:
    	if(negative_channel)
    	{
    		switch(TIM_CHANNEL_x)
    		{
    			case TIM_CHANNEL_1: BSP_GPIO_pin_config(GPIOA, 				(remap)?GPIO_PIN_11:GPIO_PIN_7, 	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF6_TIM1); break;
    			case TIM_CHANNEL_2: BSP_GPIO_pin_config((remap)?GPIOB:GPIOA,(remap)?GPIO_PIN_0:GPIO_PIN_12, 	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF6_TIM1); break;
    			case TIM_CHANNEL_3: BSP_GPIO_pin_config(GPIOF, 				GPIO_PIN_0, 						GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF6_TIM1); break;
    			default: break;
    		}
    	}
    	else
    	{
    		switch(TIM_CHANNEL_x)
    		{
    			case TIM_CHANNEL_1: BSP_GPIO_pin_config(GPIOA, 				GPIO_PIN_8, 					GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF6_TIM1); break;
    			case TIM_CHANNEL_2: BSP_GPIO_pin_config(GPIOA, 				GPIO_PIN_9, 					GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF6_TIM1); break;
    			case TIM_CHANNEL_3: BSP_GPIO_pin_config(GPIOA, 				GPIO_PIN_10, 					GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF6_TIM1); break;
    			case TIM_CHANNEL_4: BSP_GPIO_pin_config(GPIOA, 				GPIO_PIN_11, 					GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF11_TIM1); break;
    			default: break;
    		}
    	}
    	break;
    case TIMER2_ID:
    	switch(TIM_CHANNEL_x)
    	{
    		case TIM_CHANNEL_1: BSP_GPIO_pin_config(GPIOA, 				(remap)?GPIO_PIN_5:GPIO_PIN_0,	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF1_TIM2); break;
    		case TIM_CHANNEL_2: BSP_GPIO_pin_config((remap)?GPIOB:GPIOA,(remap)?GPIO_PIN_3:GPIO_PIN_1,	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF1_TIM2); break;
    		case TIM_CHANNEL_3: BSP_GPIO_pin_config(GPIOA, 				(remap)?GPIO_PIN_9:GPIO_PIN_2, 	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, (remap)?GPIO_AF10_TIM2:GPIO_AF1_TIM2); break;
    		case TIM_CHANNEL_4: BSP_GPIO_pin_config(GPIOA, 				(remap)?GPIO_PIN_10:GPIO_PIN_3,	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, (remap)?GPIO_AF10_TIM2:GPIO_AF1_TIM2); break;
    		default: break;
    	}
    	break;
    case TIMER3_ID:
    	switch(TIM_CHANNEL_x)
    	{
    		case TIM_CHANNEL_1: BSP_GPIO_pin_config((remap)?GPIOB:GPIOA,(remap)?GPIO_PIN_4:GPIO_PIN_6, 	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF2_TIM3); break;
    		case TIM_CHANNEL_2: BSP_GPIO_pin_config(GPIOA, 				(remap)?GPIO_PIN_7:GPIO_PIN_4, 	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF2_TIM3); break;
    		case TIM_CHANNEL_3: BSP_GPIO_pin_config(GPIOB, 				GPIO_PIN_0, 					GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF2_TIM3); break;
    		case TIM_CHANNEL_4: BSP_GPIO_pin_config(GPIOB, 				GPIO_PIN_7, 					GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, GPIO_AF2_TIM3); break;
    		default: break;
    	}
    	break;
    case TIMER4_ID:
    	switch(TIM_CHANNEL_x)
    	{
    	    case TIM_CHANNEL_1: BSP_GPIO_pin_config((remap)?GPIOB:GPIOA,(remap)?GPIO_PIN_6:GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, (remap)?GPIO_AF2_TIM4:GPIO_AF10_TIM4); break;
    	    case TIM_CHANNEL_2: BSP_GPIO_pin_config((remap)?GPIOB:GPIOA,(remap)?GPIO_PIN_7:GPIO_PIN_12, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, (remap)?GPIO_AF2_TIM4:GPIO_AF10_TIM4); break;
    	    case TIM_CHANNEL_3: BSP_GPIO_pin_config((remap)?GPIOB:GPIOA,(remap)?GPIO_PIN_8:GPIO_PIN_13, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH, (remap)?GPIO_AF2_TIM4:GPIO_AF10_TIM4); break;
    	    default: break;
    	}
    	break;
    default:
		break;
    }

    TIM_OC_InitTypeDef TIM_OCInitStructure = {0};
	TIM_OCInitStructure.OCMode = TIM_OCMODE_PWM1;
	TIM_OCInitStructure.Pulse = 0;
	TIM_OCInitStructure.OCPolarity = TIM_OCPOLARITY_HIGH;
	TIM_OCInitStructure.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	TIM_OCInitStructure.OCFastMode = TIM_OCFAST_DISABLE; //disable the fast state
	TIM_OCInitStructure.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	TIM_OCInitStructure.OCIdleState = TIM_OCIDLESTATE_RESET;
	HAL_TIM_PWM_Init(&structure_handles[timer_id]);
	HAL_TIM_PWM_ConfigChannel(&structure_handles[timer_id], &TIM_OCInitStructure, TIM_CHANNEL_x); //on configure le canal (avant on autorisait le prechargement de la config)
	if(negative_channel)
		HAL_TIMEx_PWMN_Start(&structure_handles[timer_id], TIM_CHANNEL_x);
	else
		HAL_TIM_PWM_Start(&structure_handles[timer_id], TIM_CHANNEL_x);
	BSP_TIMER_set_duty(timer_id, TIM_CHANNEL_x, duty);
}

/**
 * @brief Fonction de configuration du rapport cyclique
 *
 * Rapport cyclique compris entre 0 et 1000
 * 1000 = 100%
 * 0 = 0%
 * @param timer_id id du timer (cf timer_id_t)
 * @param TIM_CHANNEL_x canal de sortie
 * @param duty rapport cyclique de la PWM compris dans l'intervalle [0; 1000]
 */
void BSP_TIMER_set_duty(timer_id_t timer_id, uint16_t TIM_CHANNEL_x ,uint16_t duty)
{
	duty = MIN(duty, 1000);	// On s'assure que le duty cycle est compris entre 0 et 1000
	duty = (uint16_t)((((uint32_t)(duty))*(structure_handles[timer_id].Init.Period+1))/1000U);

	__HAL_TIM_SET_COMPARE(&structure_handles[timer_id], TIM_CHANNEL_x, duty);
}

/**
 * @brief Fonction de modification de la période de la PWM en conservant le rapport cyclique
 * @param timer_id id du timer (cf timer_id_t)
 * @param TIM_CHANNEL_x canal de sortie
 * @param uint32_t period
 */
void BSP_TIMER_set_period_with_same_duty(timer_id_t timer_id, uint16_t TIM_CHANNEL_x, uint32_t period)
{
	uint32_t previous_compare = __HAL_TIM_GET_COMPARE(&structure_handles[timer_id], TIM_CHANNEL_x);
	uint32_t previous_period = structure_handles[timer_id].Init.Period+1;
	uint32_t duty;
	BSP_TIMER_set_period(timer_id, period);

	duty = (previous_compare*1000)/previous_period;
	BSP_TIMER_set_duty(timer_id, TIM_CHANNEL_x, duty);
}

/**
 * @brief Fonctions utilisateurs appelées lors de l'interruption du timer
 *
 * L'attribut weak indique à l'éditeur de liens, lors de la compilation, que cette fonction sera ignorée s'il en existe une autre portant le même nom. Elle sera choisie par défaut d'autre fonction homonyme.
 * Ainsi, si l'utilisateur définie sa propre TIMERX_user_handler_it(), elle sera appelée
 * Sinon, aucun message d'erreur n'indiquera que cette fonction n'existe pas !
 */
__weak void TIMER1_user_handler_it(void)
{

}

__weak void TIMER2_user_handler_it(void)
{

}

__weak void TIMER3_user_handler_it(void)
{

}

__weak void TIMER4_user_handler_it(void)
{

}


__weak void TIMER6_user_handler_it(void)
{
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);
}

/**
 * @brief 	Routine d'interruption appelée AUTOMATIQUEMENT lorsque le timer 1 arrive a écheance.
 * @pre		Cette fonction NE DOIT PAS être appelée directement par l'utilisateur...
 * @post	Acquittement du flag d'interruption, et appel de la fonction de l'utilisateur : TIMER1_user_handler_it()
 * @note	Nous n'avons PAS le choix du nom de cette fonction, c'est comme ça qu'elle est nommée dans le fichier startup.s !
 */
__weak void TIM1_UP_TIM16_IRQHandler(void){
	if(__HAL_TIM_GET_IT_SOURCE(&structure_handles[TIMER1_ID], TIM_IT_UPDATE) != RESET) 	//Si le flag est levé...
	{
		__HAL_TIM_CLEAR_IT(&structure_handles[TIMER1_ID], TIM_IT_UPDATE);				//...On l'acquitte...
		TIMER1_user_handler_it();									//...Et on appelle la fonction qui nous intéresse
	}
}

void TIM2_IRQHandler(void){
	if(__HAL_TIM_GET_IT_SOURCE(&structure_handles[TIMER2_ID], TIM_IT_UPDATE) != RESET) 	//Si le flag est levé...
	{
		__HAL_TIM_CLEAR_IT(&structure_handles[TIMER2_ID], TIM_IT_UPDATE);				//...On l'acquitte...
		TIMER2_user_handler_it();									//...Et on appelle la fonction qui nous intéresse
	}
}

void TIM3_IRQHandler(void){
	if(__HAL_TIM_GET_IT_SOURCE(&structure_handles[TIMER3_ID], TIM_IT_UPDATE) != RESET) 	//Si le flag est levé...
	{
		__HAL_TIM_CLEAR_IT(&structure_handles[TIMER3_ID], TIM_IT_UPDATE);				//...On l'acquitte...
		TIMER3_user_handler_it();									//...Et on appelle la fonction qui nous intéresse
	}
}

void TIM4_IRQHandler(void){
	if(__HAL_TIM_GET_IT_SOURCE(&structure_handles[TIMER4_ID], TIM_IT_UPDATE) != RESET) 	//Si le flag est levé...
	{
		__HAL_TIM_CLEAR_IT(&structure_handles[TIMER4_ID], TIM_IT_UPDATE);				//...On l'acquitte...
		TIMER4_user_handler_it();									//...Et on appelle la fonction qui nous intéresse
	}
}

void TIM6_DAC_IRQHandler(void)
{
	if(__HAL_TIM_GET_IT_SOURCE(&structure_handles[TIMER6_ID], TIM_IT_UPDATE) != RESET) 	//Si le flag est levé...
	{
		__HAL_TIM_CLEAR_IT(&structure_handles[TIMER6_ID], TIM_IT_UPDATE);				//...On l'acquitte...
		TIMER6_user_handler_it();									//...Et on appelle la fonction qui nous intéresse
	}

}

#endif /* USE_BSP_TIMER */
