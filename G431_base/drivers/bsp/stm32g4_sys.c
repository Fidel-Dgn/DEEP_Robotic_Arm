/**
 *******************************************************************************
 * @file 	stm32g4_sys.c
 * @author 	jjo
 * @date 	Mar 11, 2024
 * @brief 	Module d'initialisation et de gestion d'erreur
 * 			Adaptation du module stm32f1_sys.c du club robot ESEO 2009
 *******************************************************************************
 */

#include "stm32g4_sys.h"
#include "stm32g4_uart.h"
#include <errno.h>
#include <sys/unistd.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef STDOUT_USART
	#define DEFAULT_STDOUT_USART UART1_ID
#endif

#ifndef STDERR_USART
	#define DEFAULT_STDERR_USART UART1_ID
#endif

#ifndef STDIN_USART
	#define DEFAULT_STDIN_USART UART1_ID
#endif

/* Private variables ---------------------------------------------------------*/
static uart_id_t stdout_usart	= DEFAULT_STDOUT_USART;
static uart_id_t stderr_usart	= DEFAULT_STDERR_USART;
static uart_id_t stdin_usart	= DEFAULT_STDIN_USART;
static volatile uint32_t uart_initialized = false;

/* Private typedef -----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private function definitions ----------------------------------------------*/

/* Public function definitions -----------------------------------------------*/

void BSP_SYS_set_std_usart(uart_id_t in, uart_id_t out, uart_id_t err)
{
	uart_initialized = 0xE5E0E5E0;
	stdin_usart = in;
	stdout_usart = out;
	stderr_usart = err;
}

/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{


  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* Configure the system clock */
  SystemClock_Config();

  /* System interrupt init*/


}



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

void BSP_SYS_HSI48_Init(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State     = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;  // pas besoin de toucher au PLL ici
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        printf("RCC Oscillator Config Error\n");
    }
}

/**
 * @brief Redefinition of _read function used e.g. by scanf
 * @param file
 * @param ptr
 * @param len
 * @return
 */
int _read(int file, char *ptr, int len)
{
	int n;
	int num = 0;
	switch (file) {
		case STDIN_FILENO:
			for (n = 0; n < len; n++)
			{
				/*while ((stdin_usart->SR & USART_FLAG_RXNE) == (uint16_t)RESET);
				char c = (char)(stdin_usart->DR & (uint16_t)0x01FF);*/
				char c;
				while(!BSP_UART_data_ready(stdin_usart));	//Blocking
				c = BSP_UART_get_next_byte(stdin_usart);
				*ptr++ = c;
				num++;
			}
			break;
		default:
			errno = EBADF;
			return -1;
	}
	return num;
}


/**
 * @brief Redefinition of _write function used e.g. by printf
 * @param file
 * @param ptr
 * @param len
 * @return
 */
int _write(int file, char *ptr, int len)
{
	int n;
	switch (file) {
		case STDOUT_FILENO: /*stdout*/
			//UART_puts(stdout_usart,ptr, len);

			for (n = 0; n < len; n++)
			{
#if TRACE
				trace_putchar(*ptr++);
#else
				BSP_UART_putc(stdout_usart,*ptr++);
#endif
			}
			break;
		case STDERR_FILENO: /* stderr */
			for (n = 0; n < len; n++)
			{
				//while ((stderr_usart->SR & USART_FLAG_TC) == (uint16_t)RESET);
				//stderr_usart->DR = (*ptr++ & (uint16_t)0x01FF);
#if TRACE
				trace_putchar(*ptr++);
#else
				BSP_UART_putc(stderr_usart,*ptr++);
#endif
			}
			break;
		default:
			errno = EBADF;
			return -1;
	}
	return len;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */


#define DUMP_PRINTF_BUFFER_SIZE	256
uint32_t dump_printf(const char *format, ...) {
	uint32_t ret;

	va_list args_list;
	va_start(args_list, format);
	uint8_t buf[DUMP_PRINTF_BUFFER_SIZE];

	ret = (uint32_t)vsnprintf((char*)buf, DUMP_PRINTF_BUFFER_SIZE, format, args_list);	//Pr�pare la chaine � envoyer.
	if(ret >= DUMP_PRINTF_BUFFER_SIZE)
		ret = DUMP_PRINTF_BUFFER_SIZE-1;

	BSP_UART_impolite_force_puts_on_uart(UART2_ID, buf, ret);

	va_end(args_list);
	return ret;
}

//TODO : Manage fault handlers as in F103 project
