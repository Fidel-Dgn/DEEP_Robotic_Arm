/**
 *******************************************************************************
 * @file 	stm32g4_uart.c
 * @author 	jjo
 * @date 	Mar 29, 2024
 * @brief	Module UART pour cible stm32g4
 * 			Adaptation du module stm32f1_uart.c de S.Poiraud et T.Bouvier
 *******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32g4_uart.h"
#include "stm32g4_sys.h"
#include "stm32g4_gpio.h"
#include "stm32g4_utils.h"
#include <stdio.h>
#include <string.h>

/*
 * Ce module logiciel permet l'utilisation des périphériques USART (Universal Synchronous/Asynchronous Receiver Transmitter)
 *
 * 	Un module UART permet d'envoyer des données (i.e. des octets) sur une "liaison série", c'est à dire sur un fil.
 * 	Les octets ainsi envoyés sont découpés en bits. Chaque bit est envoyé pendant une période fixée.
 *
 * 	Selon l'UART choisi, les broches correspondantes sont initialisées et réservé pour cet usage :
 * 		(des 'defines' permettent pour chaque USART de choisir de remapper ou non les ports Rx et Tx)
 * 	UART1 : Rx=PA10 et Tx=PA9 		ou avec remap : Rx=PB7 et Tx=PB6
 * 	UART2 : Rx=PA3 et Tx=PA2 		ou avec remap : Rx=PA15 et Tx=PA14	ou Rx=PB4 et Tx=PB3
 *
 * 	On parle de liaison série asynchrone lorsqu'aucune horloge n'accompagne la donnée pour indiquer à celui qui la reçoit l'instant où le bit est transmis.
 * 	Dans ces conditions, il faut impérativement que le récepteur sâche à quelle vitesse précise les données sont transmises.
 * 	Il "prend alors en photo" chaque bit, et reconstitue les octets.
 *
 * 	Au repos, la tension de la liaison série est à l'état logique '1'.
 * 	Pour chaque octets (8 bits) à envoyer, l'UART envoie en fait 10 bits :
 * 		1 bit de start (toujours à 0), 8 bits de données, 1 bit de stop (toujours à 1).
 * 	Ce passage par 0 avant l'envoi des données permet au récepteur de comprendre que l'on va transmettre un octet.
 * 		(sinon il ne saurait détecter le début d'un octet commencant par le bit '1' !)
 *
 * 	Voici un exemple d'utilisation de ce module logiciel.
 *
 * 	si on souhaite initialiser l'UART1, à une vitesse de 115200 bits par seconde, puis envoyer et recevoir des données sur cette liaison.
 *  Un exemple est également disponible dans la fonction UART_test()
 *
 * 	1-> Appeler la fonction BSP_UART_init(UART1_ID, 115200);
 * 	2-> Pour envoyer un octet 'A' sur l'UART1 : BSP_UART_putc(UART1_ID, 'A');
 * 	3-> Pour recevoir les octets qui auraient été reçus par l'UART1 :
 * 			if(BSP_UART_data_ready(UART1_ID))
 * 			{
 * 				uint8_t c;
 * 				c = BSP_UART_get_next_byte(UART1_ID);
 * 				//On peut faire ce qu'on souhaite avec l'octet c récupéré.
 * 			}
 * 		Les octets reçus par les périphériques UART initialisés sont traités dans une routine d'interruption, puis mémorisés dans un tableau (que l'on nomme un buffer),
 * 		jusqu'à l'appel à la fonction UART_get_next_byte().
 * 		Ce tableau peut mémoriser 128 octets. (BUFFER_RX_SIZE)
 * 		Cette méthode permet au processeur de ne pas louper des données arrivant sur ce périphérique pendant qu'il est occupé à autre chose dans le programme.
 * 		Il est simplement interrompu très brièvement pour conserver l'octet reçu, et remettre à plus tard son traitement.
 *
 * 	4-> Il est également possible de profiter de la richesse proposée par la fonction printf...
 * 		qui permet d'envoyer un texte 'variable', constitué avec une chaine de format et des paramètres.
 * 			Pour cela :
 * 			Appelez au moins une fois, lors de l'initialisation, la fonction
 * 				BSP_SYS_set_std_usart(UART1_ID, UART1_ID, UART1_ID);   //indique qu'il faut utiliser l'UART1 pour sortir les données du printf.
 * 			Puis :
 * 				uint32_t millivolt = 3245;	//une façon éléguante d'exprimer le nombre 3,245 Volts
 * 				printf("Bonjour le monde, voici un joli nombre à virgule : %d,%03d V\n", millivolt/1000, millivolt%1000);
 *
 */

//Les buffers de réception accumulent les données reçues, dans la limite de leur taille.
//Les emplacement occupés par les octets reçus sont libérés dès qu'on les consulte.
#define BUFFER_RX_SIZE	128
#define UART_TIMEOUT 1000

static UART_HandleTypeDef structure_handles[UART_ID_NB];	//Ce tableau contient les structures qui sont utilisées pour piloter chaque UART avec la librairie HAL.
static const USART_TypeDef * instances_array[UART_ID_NB] = {USART1, USART2};
static const IRQn_Type nvic_IRQ_array[UART_ID_NB] = {USART1_IRQn, USART2_IRQn};

//Buffers
static uint8_t buffer_rx[UART_ID_NB][BUFFER_RX_SIZE];
static uint8_t buffer_rx_write_index[UART_ID_NB] = {0};
static uint32_t buffer_rx_read_index[UART_ID_NB] = {0};
static volatile bool buffer_rx_data_ready[UART_ID_NB] = {false};
static volatile bool uart_initialized[UART_ID_NB] = {false};
static rx_callback_fun_t callback_uart_rx[UART_ID_NB] = {NULL};

/**
 * @brief Cette fonction blocante a pour but de vous aider à appréhender les fonctionnalités de ce module logiciel.
 *
 * Complètement inutile, cette fonction accumule les octets reçus dans un tableau,
 * puis les renvoie sur l'UART2 dès qu'un caractère '\n' est reçu.
 */
void BSP_UART_demo(void)
{
	#define DEMO_TAB_SIZE 128

	static uint8_t tab[DEMO_TAB_SIZE];
	static uint16_t index = 0;
	uint8_t c;
	while(1)
	{
		if(BSP_UART_data_ready(UART2_ID))
		{
			c = BSP_UART_getc(UART2_ID);			//lecture du prochain caractère
			tab[index] = c;							//On mémorise le caractère dans le tableau
			if(c=='\n')								//Si c'est la fin de la chaine
			{
				tab[index+1] = 0; 					//fin de chaine, en écrasant le caractère suivant par un 0
				BSP_UART_puts(UART2_ID, tab, 0);	//on renvoie la chaine reçue.
				index = 0;							//Remise à zéro de l'index
			}
			else if(index < DEMO_TAB_SIZE - 2)
			{										//Pour tout caractère différent de \n
				index++;							//on incrémente l'index (si < TAB_SIZE -2 !)
			}
		}
	}
}


/**
 * @brief Fonction permettant de savoir si le buffer de l'UART demandé est vide ou non.
 *
 * @param uart_id ID de l'uart concerné
 * @ret bool true si des caractères sont disponibles, false sinon
 */
bool BSP_UART_data_ready(uart_id_t uart_id)
{
	assert(uart_id < UART_ID_NB);
	return buffer_rx_data_ready[uart_id];
}

uint32_t BSP_UART_get_nb_data_ready(uart_id_t uart_id)
{
	assert(uart_id < UART_ID_NB);
	if(!buffer_rx_data_ready[uart_id])
		return 0;
	if(buffer_rx_write_index[uart_id] >= buffer_rx_read_index[uart_id])
		return buffer_rx_write_index[uart_id] - buffer_rx_read_index[uart_id];
	else
		return BUFFER_RX_SIZE - (buffer_rx_read_index[uart_id] - buffer_rx_write_index[uart_id]);
}

/**
 * @brief Simulation d'un bouton par un appui clavier
 *
 * N'importe quelle touche peut être utilisée
 * @param uart_id ID de l'uart concerné
 * @return true si un appui de touche a eu lieu
 */
bool BSP_UART_button(uart_id_t uart_id)
{
        if( BSP_UART_data_ready(uart_id) )      /* Si un caractère est reçu sur l'UART 2*/
        {
                /* On "utilise" le caractère pour vider le buffer de réception */
                if (BSP_UART_get_next_byte(uart_id) == ESCAPE_KEY_CODE)
                	return true;
                else
                	return false;
        }
        else
                return false;
}

/**
 * @brief Fonction permettant de lire le prochain caractère reçu sur l'UART demandé
 *
 * @param uart_id ID de l'uart concerné
 * @return uint8_t le dernier caractère reçu? Ou 0 si rien n'a été reçu
 * @post Le caractère lu est retiré du buffer de réception
 */
uint8_t BSP_UART_get_next_byte(uart_id_t uart_id)
{
	uint8_t ret;
	assert(uart_id < UART_ID_NB);

	if(!buffer_rx_data_ready[uart_id])	//N'est jamais sensé se produire si l'utilisateur vérifie que BSP_UART_data_ready() avant d'appeler UART_get_next_byte()
		return 0;

	ret =  buffer_rx[uart_id][buffer_rx_read_index[uart_id]];
	buffer_rx_read_index[uart_id] = (buffer_rx_read_index[uart_id] + 1) % BUFFER_RX_SIZE;

	//Section critique durant laquelle on désactive les interruptions... pour éviter une mauvaise préemption.
	NVIC_DisableIRQ(nvic_IRQ_array[uart_id]);
	if (buffer_rx_write_index[uart_id] == buffer_rx_read_index[uart_id])
		buffer_rx_data_ready[uart_id] = false;
	NVIC_EnableIRQ(nvic_IRQ_array[uart_id]);
	return ret;
}

/**
 * @brief Fonction permettant de lire le prochain caractère reçu sur l'UART demandé
 *
 * @param uart_id ID de l'uart concerné
 * @return uint8_t le dernier caractère reçu? Ou 0 si rien n'a été reçu
 * @post Le caractère lu est retiré du buffer de réception
 */
uint8_t BSP_UART_getc(uart_id_t uart_id)
{
	return BSP_UART_get_next_byte(uart_id);
}



/**
 * @func 	uint8_t BSP_UART_getc_blocking(uart_id_e uart_id, uint32_t timeout))
 * @brief	Fonction blocante !! qui retourne le dernier caractere re�u sur l'USARTx. Ou 0 si pas de caractere re�u au del� du timeout
 * @param	UART_Handle : UART_Handle.Instance = USART1, USART2 ou USART6
 * @param	timeout au del� duquel on abandonne le blocage, sauf si timeout vaut 0 (attente infinie)
 * @post	Si le caractere re�u est 0, il n'est pas possible de faire la difference avec le cas o� aucun caractere n'est re�u.
 * @ret		Le caractere re�u, sur 8 bits.
 */
uint8_t BSP_UART_getc_blocking(uart_id_t uart_id, uint32_t timeout)
{
	uint32_t initial = HAL_GetTick();
	uint8_t c = 0;
	do
	{
		if(BSP_UART_data_ready(uart_id))
		{
			c = BSP_UART_get_next_byte(uart_id);
			break;
		}
	}while(timeout==0 || HAL_GetTick() - initial < timeout);
	return c;
}


/**
 * @func 	uint32_t BSP_UART_gets_blocking(uart_id_e uart_id, uint8_t * datas, uint32_t len, uint32_t timeout)
 * @brief	Fonction blocante !! qui retourne "len" caracteres re�us sur l'USARTx
 * @param	les caract�res re�us sont rang�s dans le buffer datas.
 * @param	UART_Handle : UART_Handle.Instance = USART1, USART2 ou USART6
 * @param	timeout au del� duquel on abandonne le blocage, sauf si timeout vaut 0 (attente infinie)
 * @ret		Le nombre de caracteres re�us
 */
uint32_t BSP_UART_gets_blocking(uart_id_t uart_id, uint8_t * datas, uint32_t len, uint32_t timeout)
{
	uint32_t i;
	uint32_t initial = HAL_GetTick();
	for(i=0; i<len ; i++)
	{
		if(BSP_UART_data_ready(uart_id))
			datas[i] = BSP_UART_get_next_byte(uart_id);
		else
		{
			while(!BSP_UART_data_ready(uart_id) || timeout==0 || (HAL_GetTick() - initial < timeout));
		}
	}
	return i;
}

/**
 * @brief	Lit "len" caractères reçus, s'ils existent...
 *
 * @post	Fonction non blocante : s'il n'y a plus de caractère reçu, cette fonction renvoit la main
 * @return		Le nombre de caractères lus.
 */
uint32_t BSP_UART_gets(uart_id_t uart_id, uint8_t * datas, uint32_t len)
{
	uint32_t i;
	for(i=0; i<len ; i++)
	{
		if(BSP_UART_data_ready(uart_id))
			datas[i] = BSP_UART_get_next_byte(uart_id);
		else
			break;
	}
	return i;
}

/**
 * @brief	Envoi un caractere sur l'UARTx. Fonction BLOCANTE si un caractere est deja en cours d'envoi.
 *
 * @param	c : le caractere a envoyer
 * @param	uart_id UART1_ID, UART2_ID
 */
void BSP_UART_putc(uart_id_t uart_id, uint8_t c)
 {
	HAL_StatusTypeDef state;
	assert(uart_id < UART_ID_NB);
	if(uart_initialized[uart_id])
	{
		do
		{
			NVIC_DisableIRQ(nvic_IRQ_array[uart_id]);
			state = HAL_UART_Transmit(&structure_handles[uart_id], &c, 1, UART_TIMEOUT);
			NVIC_EnableIRQ(nvic_IRQ_array[uart_id]);
		}while(state == HAL_BUSY);
	}
}

/**
 * @brief	Envoi une chaine de caractere sur l'USARTx. Fonction BLOCANTE si un caractere est deja en cours d'envoi.
 *
 * @param	uart_id : UART1_ID, UART2_ID
 * @param	str : la chaine de caractère à envoyer
 * @param	len : le nombre de caractères à envoyer. Si 0, la longueur de la chaîne est évaluée dynamiquement avec strlen
 */
void BSP_UART_puts(uart_id_t uart_id, const uint8_t *str, uint16_t len)
{
    HAL_StatusTypeDef state;
    HAL_UART_StateTypeDef uart_state;
    assert(uart_id < UART_ID_NB);
	if(uart_initialized[uart_id])
	{
		if (len == 0)
			len = strlen((const char*) str);
		do
		{
			NVIC_DisableIRQ(nvic_IRQ_array[uart_id]);
			state = HAL_UART_Transmit(&structure_handles[uart_id], (uint8_t*) str, len, UART_TIMEOUT);
			NVIC_EnableIRQ(nvic_IRQ_array[uart_id]);
		}while(state == HAL_BUSY);

		do
		{
			uart_state = HAL_UART_GetState(&structure_handles[uart_id]);
		}while(uart_state == HAL_UART_STATE_BUSY_TX || uart_state == HAL_UART_STATE_BUSY_TX_RX);
	}
}

/**
 * @brief	Initialise l'USARTx - 8N1 - vitesse des bits (baudrate) indiqué en paramètre
 * @func	void UART_init(uint8_t uart_id, uart_interrupt_mode_e mode)
 * @param	uart_id est le numéro de l'UART concerné :
 * 				UART1_ID
 * 				UART2_ID
 * @param	baudrate indique la vitesse en baud/sec
 * 				115200	vitesse proposée par défaut
 * 				9600	vitesse couramment utilisée
 * 				19200	...
 * @post	Cette fonction initialise les broches suivante selon l'USART choisit en parametre :
 * 				USART1 : Rx=PA10 et Tx=PA9 		ou avec remap : Rx=PB7 et Tx=PB6
 * 				USART2 : Rx=PA3 et Tx=PA2 		ou avec remap : Rx=PA15 et Tx=PA14	ou Rx=PB4 et Tx=PB3
 * 				La gestion des envois et reception se fait en interruption.
 *
 */
void BSP_UART_init(uart_id_t uart_id, uint32_t baudrate)
{
	assert(baudrate > 1000);
	assert(uart_id < UART_ID_NB);

	buffer_rx_read_index[uart_id] = 0;
	buffer_rx_write_index[uart_id] = 0;
	buffer_rx_data_ready[uart_id] = false;
	/* UARTx configured as follow:
		- Word Length = 8 Bits
		- One Stop Bit
		- No parity
		- Hardware flow control disabled (RTS and CTS signals)
		- Receive and transmit enabled
		- OverSampling: enable
		- One bit sampling: disable
		- Prescaler: DIV1
		- Advance features: disabled
	*/
	structure_handles[uart_id].Instance = (USART_TypeDef*)instances_array[uart_id];
	structure_handles[uart_id].Init.BaudRate = baudrate;
	structure_handles[uart_id].Init.WordLength = UART_WORDLENGTH_8B;
	structure_handles[uart_id].Init.StopBits = UART_STOPBITS_1;
	structure_handles[uart_id].Init.Parity = UART_PARITY_NONE;
	structure_handles[uart_id].Init.HwFlowCtl = UART_HWCONTROL_NONE;
	structure_handles[uart_id].Init.Mode = UART_MODE_TX_RX;
	structure_handles[uart_id].Init.OverSampling = UART_OVERSAMPLING_16;
	structure_handles[uart_id].Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	structure_handles[uart_id].Init.ClockPrescaler = UART_PRESCALER_DIV1;
	structure_handles[uart_id].AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	if (HAL_UART_Init(&structure_handles[uart_id]) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(&structure_handles[uart_id], UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(&structure_handles[uart_id], UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(&structure_handles[uart_id]) != HAL_OK)
	{
		Error_Handler();
	}

	/* Interrupt Init */
	HAL_NVIC_SetPriority(nvic_IRQ_array[uart_id], 1, 1);
	HAL_NVIC_EnableIRQ(nvic_IRQ_array[uart_id]);
	HAL_UART_Receive_IT(&structure_handles[uart_id],&buffer_rx[uart_id][buffer_rx_write_index[uart_id]],1);	//Activation de la réception d'un caractère

	//Config LibC: no buffering
	setvbuf(stdout, NULL, _IONBF, 0 );
	setvbuf(stderr, NULL, _IONBF, 0 );
	setvbuf(stdin, NULL, _IONBF, 0 );

	uart_initialized[uart_id] = true;
}

void HAL_UART_MspInit(UART_HandleTypeDef* uart_handle)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	if(uart_handle->Instance==USART1)
	{
		/** Initializes the peripherals clocks
		*/
		PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
		PeriphClkInit.Usart2ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
		{
		  Error_Handler();
		}

#ifdef UART1_ON_PA10_PA9
		__HAL_RCC_GPIOA_CLK_ENABLE();
		BSP_GPIO_pin_config(GPIOA, GPIO_PIN_9|GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART1);
#endif
#ifdef UART1_ON_PB7_PB6
		__HAL_RCC_GPIOB_CLK_ENABLE();
		BSP_GPIO_pin_config(GPIOB, GPIO_PIN_6|GPIO_PIN_7, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART1);
#endif
		/* UART1 clock enable */
		__HAL_RCC_USART1_CLK_ENABLE();
	}

	if(uart_handle->Instance==USART2)
	{
		/** Initializes the peripherals clocks
		*/
		PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
		PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
		{
		  Error_Handler();
		}

#ifdef UART2_ON_PA3_PA2
		__HAL_RCC_GPIOA_CLK_ENABLE();
		BSP_GPIO_pin_config(GPIOA, GPIO_PIN_2 | GPIO_PIN_3, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART2);
#endif
#ifdef UART2_ON_PA15_PA14
		__HAL_RCC_GPIOA_CLK_ENABLE();
		BSP_GPIO_pin_config(GPIOA, GPIO_PIN_14 | GPIO_PIN_15, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART2);
#endif
#ifdef UART2_ON_PB4_PB3
		__HAL_RCC_GPIOB_CLK_ENABLE();
		BSP_GPIO_pin_config(GPIOB, GPIO_PIN_3 | GPIO_PIN_4, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF7_USART2);
#endif

		/* UART2 clock enable */
		__HAL_RCC_USART2_CLK_ENABLE();
	}
}

/**
 * @brief	Déinitialise l'UARTx
 *
 * @param	uart_id est le numéro de l'UART concerné :	UART1_ID, UART2_ID
 */
void BSP_UART_deinit(uart_id_t uart_id)
{
	assert(uart_id < UART_ID_NB);
	HAL_UART_DeInit(&structure_handles[uart_id]);

    /* UART2 interrupt Deinit */
	HAL_NVIC_DisableIRQ(nvic_IRQ_array[uart_id]);

	uart_initialized[uart_id] = false;
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uart_handle)
{
	if (uart_handle->Instance == USART1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_USART1_CLK_DISABLE();

#ifdef UART1_ON_PA10_PA9
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
#endif
#ifdef UART1_ON_PB7_PB6
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);
#endif
	}
	if(uart_handle->Instance==USART2)
	{

    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

#ifdef UART2_ON_PA3_PA2
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
#endif
#ifdef UART2_ON_PA15_PA14
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_14|GPIO_PIN_15);
#endif
#ifdef UART2_ON_PB4_PB3
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_4);
#endif

	}
}

void USART1_IRQHandler(void)
{
	HAL_UART_IRQHandler(&structure_handles[UART1_ID]);
}

void USART2_IRQHandler(void)
{
	HAL_UART_IRQHandler(&structure_handles[UART2_ID]);
}

/**
 * @brief Affecte une fonction de callback sur réception d'un caractère UART
 *
 * @param uart_id ID de l'uart concerné
 * @param cb pointeur sur la fonction de callback à ajouter (nom de la fonction)
 */
void BSP_UART_set_callback(uart_id_t uart_id, rx_callback_fun_t cb)
{
	assert(uart_id < UART_ID_NB);
	callback_uart_rx[uart_id] = cb;
}

#define USART_FLAG_ERRORS (USART_ISR_ORE | USART_ISR_NE | USART_ISR_FE | USART_ISR_PE)
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	__unused uint8_t trash;
	uint32_t status;
	do{
		status = huart->Instance->ISR;
		if (status & USART_ISR_RXNE)
			trash = (uint8_t) (huart->Instance->RDR);
		if (status & USART_FLAG_ERRORS)
			huart->Instance->ICR = USART_FLAG_ERRORS;
	}while(status & USART_FLAG_ERRORS);
}


/**
 * @brief Cette fonction est appelée en interruption UART par le module HAL.
 *
 * @param huart handle de l'UART concerné
 * @param Size	Le nombre d'octet à traiter
 * @post Les octets reçus sont stockés dans le buffer correspondant.
 * @post La réception en IT des prochains octets est réactivée.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_id_t uart_id;
	if (huart->Instance == USART1)
		uart_id = UART1_ID;
	else if (huart->Instance == USART2)
		uart_id = UART2_ID;
	else
		return;

	if (callback_uart_rx[uart_id] != NULL)
	{
		callback_uart_rx[uart_id]((char)buffer_rx[uart_id][0]);
		HAL_UART_Receive_IT(&structure_handles[uart_id], &buffer_rx[uart_id][0], 1);//Activation de la réception d'un caractère
	}
	else
	{
		buffer_rx_data_ready[uart_id] = true;
		buffer_rx_write_index[uart_id] = (buffer_rx_write_index[uart_id] + 1) % BUFFER_RX_SIZE;
		HAL_UART_Receive_IT(&structure_handles[uart_id], &buffer_rx[uart_id][buffer_rx_write_index[uart_id]], 1);//Activation de la réception d'un caractère
	}
}

//ecriture impolie forcée bloquante sur l'UART (à utiliser en IT, en cas d'extrême recours)
void BSP_UART_impolite_force_puts_on_uart(uart_id_t uart_id, uint8_t * str, uint32_t len)
{
	assert(uart_id < UART_ID_NB);
	uint32_t i;
	if(uart_initialized[uart_id])
	{
		USART_TypeDef * pusart;
		pusart = structure_handles[uart_id].Instance;
		for(i=0; i<len; i++)
		{
			while((pusart->ISR & USART_ISR_TXE) == 0);
			pusart->TDR = (uint16_t)(str[i]);
		}
	}
}


