/*
 * QS_bootloader.c
 *
 *  Created on: May 13, 2024
 *      Author: Nirgal
 */
#pragma GCC push_options
#pragma GCC optimize ("-Og")

//ATTENTION, il est interdit d'inclure un autre fichier ici. On se d�merde int�gralement avec nous m�me.
#include "stm32g4xx.h"
#include "bootloader.h"



//Les variables static sont �galement interdites !
//Toutes les fonctions de ce fichier devront �tre plac�es dans la section .bootloader (� l'aide de la macro bl_func !)

#define NULL ((void *)0)

#define SOH 									0x01
#define EOT 									0x04
#define SID_TOASTER_REQUEST_FOR_PROGRAM 		0x70
#define SID_BOOTLOADER_PROGRAM_AVAILABLE 		0x71
#define SID_BOOTLOADER_PROGRAM_NOT_AVAILABLE 	0x72
#define SID_TOASTER_ASK_FOR_PACKET 				0x73
#define SID_TOASTER_PACKET 						0x74
#define PACKET_DATA_SIZE 						16384

typedef struct
{
	uint8_t sid;
	uint8_t size;
	uint8_t data[8];
}msg_t;

typedef struct
{
	uint32_t size;
	uint32_t crc;
	uint32_t crc_calculated;
	uint8_t packet_nb;
	uint8_t data[PACKET_DATA_SIZE];
}packet_t;

#define bl_func __attribute__((section(".bootloader"))) static

bl_func FLASH_Status FLASH_write_packet(uint32_t * address, packet_t * packet);
bl_func void BL_FLASH_Erase(uint32_t program_size);
bl_func void Unlock(void);
bl_func void Lock(void);
bl_func FLASH_Status WaitForLastOperation(void);
bl_func FLASH_Status GetStatus(void);
bl_func void UART_write(uint8_t data);
bl_func uint8_t UART_read(uint8_t * c);
bl_func uint8_t TOASTER_receive(msg_t * msg, packet_t * packet, uint32_t timeout_nb_loops);
bl_func uint8_t TOASTER_receive_B0(uint32_t timeout_nb_loops);
bl_func void TOASTER_send_request_for_program(void);
bl_func void TOASTER_ask_for_packet(uint8_t packet_number);
bl_func void msgToUART(msg_t * msg);

//seule fonction publique de ce fichier !
__attribute__((section(".bootloader.begin"))) void bootloader(uint32_t version_of_toaster)
{
	//return;
	RCC->AHB1ENR |= RCC_AHB1ENR_FLASHEN;
	RCC->APB2ENR = 1;
	RCC->APB1ENR1 |= 1<<28;
	RCC->ICSCR = 0x40950000;
    RCC->PLLCFGR = 0x10005532;
    RCC->CR = 0x03000500;
    RCC->PLLCFGR = 0x11005532;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U);
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_LATENCY_4);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_SYSCLK_DIV2);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, 0x03);
    while (__HAL_RCC_GET_SYSCLK_SOURCE() != (0x03 << RCC_CFGR_SWS_Pos));
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_HCLK_DIV16);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_HCLK_DIV16);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, 0x00);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, 0x00);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, ((0x00) << 3U));

    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);	//GPIOA CLK ON
    __HAL_RCC_USART1_CONFIG(0);
    GPIOA->AFR[1]=0x0770;
    GPIOA->MODER=0xABEBFFFF;
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN); //USART1 CLK ON
    USART1->CR1 &= ~USART_CR1_UE;		//USART1 OFF
    USART1->CR1 = 0x0000000C;
    USART1->CR2 = 0x00000000;
    USART1->CR3 = 0x00000000;
    USART1->BRR = 0x00000171;
    USART1->PRESC = 0x00000000;
    CLEAR_BIT(USART1->CR2, (USART_CR2_LINEN | USART_CR2_CLKEN));
    CLEAR_BIT(USART1->CR3, (USART_CR3_SCEN | USART_CR3_HDSEL | USART_CR3_IREN));
    USART1->CR1 |= USART_CR1_UE;		//USART1 ON

	msg_t msg;
	uint32_t nb_packets;
	uint32_t program_size;
	uint32_t toaster_version_available;

	if(TOASTER_receive_B0(400000) == 0)
	{
		USART1->CR1 = 0;
		RCC->APB2ENR = 0;
		RCC->APB1ENR1 &= ~(1<<28);
		CLEAR_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);	//GPIOA CLK OFF
		CLEAR_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN); 	//USART1 CLK OFF
		return;
	}

	//on envoie un message � l'TOASTER pour demander si un programme nous attend.
	TOASTER_send_request_for_program();

	//on attend une r�ponse pendant 10ms
	uint8_t res;
	res = TOASTER_receive(&msg, NULL, 40000000);//40000);	//fonction blocante avec timeout.
	//TODO r�duire le timeout lorsque le TOASTER sera correctement impl�ment�...

	if(res == 1 && msg.sid == SID_BOOTLOADER_PROGRAM_AVAILABLE && msg.size >= 8)
	{
		toaster_version_available = msg.data[0];
		nb_packets = U32FROMU8(0x00, msg.data[1], msg.data[2], msg.data[3]);
		program_size = U32FROMU8(msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
	}
	else
		return;    //pas de programme � charger.

	if(version_of_toaster == toaster_version_available)
		return;	//on dispose d�j� de la version qui correspond � notre bluepill toaster... --> on rejoint le programme.

	if (program_size > 120*1024)
		return;	//programme trop gros, on ne peut pas le charger.

	Unlock();
	//Erase la flash (en fonction de la taille du programme � recevoir)
	BL_FLASH_Erase(program_size);

	FLASH_Status status;
	packet_t packet;
	for(int p = nb_packets-1; p>=0; )	//pas de p-- ici, on d�cr�mente qu'apr�s v�rification de validit� du packet.
	{
		//demander le packet (en partant du dernier, pour terminer par le d�but, moins risqu� en cas de coupure de communication ou de courant imminente)
		TOASTER_ask_for_packet(p);

		//r�ception du packet
		if(TOASTER_receive(&msg, &packet, 400000000) && msg.sid == SID_TOASTER_PACKET)
		{	//Si ok :
			//�crire le packet dans la flash
			uint32_t * address;
			address = (uint32_t *)(0x08000000 + p * PACKET_DATA_SIZE);

			status = FLASH_write_packet(address, &packet);

			if (status == BL_FLASH_COMPLETE)
				p--;
		}
	}

	Lock();

	while((USART1->ISR & USART_ISR_TC) == 0);	//attendre que la transmission soit termin�e !
	RCC->APB2ENR = 0;
	RCC->APB1ENR1 &= ~(1<<28);
	CLEAR_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);	//GPIOA CLK OFF
	CLEAR_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN); 	//USART1 CLK OFF
	//Fini, on reset !
	__asm volatile ("dsb");   /* Ensure all outstanding memory accesses included buffered write are completed before reset */
	SCB->AIRCR  = ((0x5FA << 16) | (SCB->AIRCR & (7<<8)) | (1UL << 2));    /* Keep priority group unchanged */
	__asm volatile ("dsb");
	return;
	//test_flash();
}


int test(int i)
{
	return 4 + i;
}



void UART_write(uint8_t c)
{
	while((USART1->ISR & USART_ISR_TXE) == 0);
	USART1->TDR = (uint16_t)(c);
}

#define USART_FLAG_ERRORS (USART_ISR_ORE | USART_ISR_NE | USART_ISR_FE | USART_ISR_PE)
uint8_t UART_read(uint8_t * c)
{
	uint32_t status;
	uint8_t ret = 0;
	do{
		status = USART1->ISR;
		if (status & USART_ISR_RXNE)
		{
			ret = 1;
			*c = (uint8_t) (USART1->RDR);
		}
		if (status & USART_FLAG_ERRORS)
			USART1->ICR = USART_FLAG_ERRORS;
	}while(status & USART_FLAG_ERRORS);

	return ret;
}

void TOASTER_send_request_for_program(void)
{
	msg_t msg;
	msg.sid = SID_TOASTER_REQUEST_FOR_PROGRAM;
	msg.size = 4;
	msg.data[0] = PACKET_DATA_SIZE>>24 & 0xFF;
	msg.data[1] = PACKET_DATA_SIZE>>16 & 0xFF;
	msg.data[2] = PACKET_DATA_SIZE>>8 & 0xFF;
	msg.data[3] = PACKET_DATA_SIZE & 0xFF;
	msgToUART(&msg);
}

void TOASTER_ask_for_packet(uint8_t packet_number)
{
	msg_t msg;
	msg.sid = SID_TOASTER_ASK_FOR_PACKET;
	msg.size = 1;
	msg.data[0] = packet_number;
	msgToUART(&msg);
}

uint8_t TOASTER_receive_B0(uint32_t timeout_nb_loops)
{
	uint8_t c;
	do{
		if(UART_read(&c))
		{
			if(c == 0xB0)
				return 1;
		}
	}while(timeout_nb_loops--);
	return 0;
}



uint8_t TOASTER_receive(msg_t * msg, packet_t * packet, uint32_t timeout_nb_loops)
{
	typedef enum{
		WAIT_SOH,
		WAIT_SID,
		WAIT_SIZE,
		RECEIVE_DATA,
		RECEIVE_PACKET,
		WAIT_EOT
		}state_e;
	state_e state;
	uint8_t remaining_data;
	uint8_t c;

	state = WAIT_SOH;

	uint32_t packet_index;
	do{
		if(UART_read(&c))
		{
			switch(state)
			{
				case WAIT_SOH:
					if(c == SOH)
						state = WAIT_SID;
					break;
				case WAIT_SID:
					msg->sid = c;
					state = WAIT_SIZE;
					break;
				case WAIT_SIZE:
					msg->size = c;
					remaining_data = c;
					if(msg->size == 0)
						state = WAIT_EOT;
					else if(msg->size <= 8)
						state = RECEIVE_DATA;
					else
						state = WAIT_SOH;
					break;
				case RECEIVE_DATA:
					msg->data[msg->size - remaining_data] = c;
					remaining_data--;
					if(remaining_data == 0)
					{
						state = WAIT_EOT;
						if(msg->sid == SID_TOASTER_PACKET && msg->size == 8)
						{
							if(packet != NULL)
							{
								state = RECEIVE_PACKET;
								packet_index = 0;
								packet->packet_nb = msg->data[0];
								packet->size = U32FROMU8(0x00, msg->data[1], msg->data[2], msg->data[3]);
								packet->crc = U32FROMU8(msg->data[4], msg->data[5], msg->data[6], msg->data[7]);
								packet->crc_calculated = 0;
							}
						}
					}
				break;
				case RECEIVE_PACKET:
					packet->data[packet_index] = c;
					packet_index++;
					if(packet_index % 4 == 0)
					{
						packet->crc_calculated ^= U32FROMU8(packet->data[packet_index-1], packet->data[packet_index-2], packet->data[packet_index-3], packet->data[packet_index-4]);
					}
					if (packet_index == packet->size)
					{
						if(packet->crc == packet->crc_calculated)
							state = WAIT_EOT;
						else
							state = WAIT_SOH;
					}
					break;
				case WAIT_EOT:
					if(c == EOT)
						return 1;
					else if(c == SOH)
						state = WAIT_SID;
					else
						state = WAIT_SOH;
				break;

				default:
					state = WAIT_SOH;
				break;
			}
		}
	}while(timeout_nb_loops--);

	return 0;
}


void msgToUART(msg_t * msg)
{
	uint8_t j = 0;
	UART_write(SOH);
	UART_write(msg->sid);
	UART_write(msg->size);
	for (j=0; j<msg->size && j<8; j++)
		UART_write(msg->data[j]);
	UART_write(EOT);
}


#define FLASH_START_ADDRESS  ((int)0x08000000)

static void BL_FLASH_Erase(uint32_t program_size)
{
	uint32_t last_used_sector;
	last_used_sector = ((program_size/PACKET_DATA_SIZE+1)*PACKET_DATA_SIZE / 0x800) + 1;
	//Les secteurs 0 et 1 ne sont pas effac�s, ils contiennent le startup et le bootloader.
		//le startup sera effac� au dernier moment, lors de son �crasement, pour ne pas risquer de perdre le lien vers le bootloader.
		//le bootloader n'est jamais �cras� par le bootloader lui-m�me...
	//on efface les suivants si n�cessaires selon l'adresse de fin du programme.
	for(uint32_t s = 2; s<=last_used_sector; s++)
	{
		MODIFY_REG(FLASH->CR, FLASH_CR_PNB, (s << FLASH_CR_PNB_Pos));
		FLASH->CR |= FLASH_CR_PER;
		FLASH->CR |= FLASH_CR_STRT;
		while(GetStatus() == BL_FLASH_BUSY);
		CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));
	}
}

static FLASH_Status FLASH_write_packet(uint32_t * address, packet_t * packet)
{
	FLASH_Status status;
	uint32_t * data;
	status = WaitForLastOperation();

	if(address == (uint32_t *)(0x08000000))
	{
		MODIFY_REG(FLASH->CR, FLASH_CR_PNB, (0 << FLASH_CR_PNB_Pos));
		FLASH->CR |= FLASH_CR_PER;
		FLASH->CR |= FLASH_CR_STRT;		//on efface le secteur 0
		while(GetStatus() == BL_FLASH_BUSY);
		CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));
	}

	if(status == BL_FLASH_COMPLETE)
	{
		//__HAL_FLASH_DATA_CACHE_DISABLE();


		  /* Deactivate the data cache if they are activated to avoid data misbehavior */
		    if(READ_BIT(FLASH->ACR, FLASH_ACR_DCEN) != 0U)
		    {
		      /* Disable data cache  */
		      __HAL_FLASH_DATA_CACHE_DISABLE();
		      //pFlash.CacheToReactivate = FLASH_CACHE_DCACHE_ENABLED;
		    }

		    /* Flush the caches to be sure of the data consistency */
//		    FLASH_FlushCaches();

		/* if the previous operation is completed, proceed to program the new data */
	//	FLASH->CR &= CR_PSIZE_MASK;
	//	FLASH->CR |= FLASH_PSIZE_WORD;
		FLASH->CR |= FLASH_CR_PG;

		data = (uint32_t *)packet->data;
		volatile uint32_t * a;
		for(uint32_t i = 0; i<packet->size/8; i++)
		{
			a = (uint32_t*)(address+2*i);
			if (a < (uint32_t *)0x08000800 || a >= (uint32_t *)0x08001000)	//on �crase pas le secteur 1 (bootloader)!
			{
				//uint32_t dh = U32FROMU8(data[4*i], data[4*i+1], data[4*i+2], data[4*i+3]);
				//uint32_t dl = U32FROMU8(data[4*i+3], data[4*i+2], data[4*i+1], data[4*i]);
				*a = data[2*i];
				__ISB();
				*(a+1) = data[2*i+1];
				status = WaitForLastOperation();
				if(*a != data[2*i] || *(a+1) != data[2*i+1])
					status = BL_FLASH_ERROR_PROGRAM;
			}
			if (status != BL_FLASH_COMPLETE)
				break;
		}
		/* if the program operation is completed, disable the PG Bit */
		FLASH->CR &= (~FLASH_CR_PG);
	}
	/* Return the Program Status */
	return status;
}

static void Unlock(void)
{
  if((FLASH->CR & FLASH_CR_LOCK) != 0)
  {
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
  }
}

static void Lock(void)
{
  FLASH->CR |= FLASH_CR_LOCK;
}

static FLASH_Status WaitForLastOperation(void)
{
  volatile FLASH_Status status;
  do{
	  status = GetStatus();
  }while(status == BL_FLASH_BUSY);

  return status;
}


static FLASH_Status GetStatus(void)
{
	uint32_t sr;
	sr = FLASH->SR;
	if(sr & FLASH_FLAG_BSY)
		return BL_FLASH_BUSY;
	if(sr & FLASH_FLAG_WRPERR)
		return BL_FLASH_ERROR_WRP;
	if(sr & (uint32_t)0xEF)
		return BL_FLASH_ERROR_PROGRAM;
	if(sr & FLASH_FLAG_OPERR)
		return BL_FLASH_ERROR_OPERATION;
	return BL_FLASH_COMPLETE;
}

#pragma GCC pop_options



