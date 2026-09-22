/*
 * QS_bootloader.h
 *
 *  Created on: May 13, 2024
 *      Author: Nirgal
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

//ce fichier ne doit être inclus QUE PAR son .c

typedef enum
{
  BL_FLASH_BUSY = 1,
  BL_FLASH_ERROR_PGS,
  BL_FLASH_ERROR_PGP,
  BL_FLASH_ERROR_PGA,
  BL_FLASH_ERROR_WRP,
  BL_FLASH_ERROR_PROGRAM,
  BL_FLASH_ERROR_OPERATION,
  BL_FLASH_ERROR_CRC,
  BL_FLASH_COMPLETE
}FLASH_Status;



#define CR_PSIZE_MASK              ((uint32_t)0xFFFFFCFF)


#define RDP_KEY                  ((uint16_t)0x00A5)



#define GPIO_Pin_0                 ((uint16_t)0x0001)  /* Pin 0 selected */
#define GPIO_Pin_1                 ((uint16_t)0x0002)  /* Pin 1 selected */
#define GPIO_Pin_2                 ((uint16_t)0x0004)  /* Pin 2 selected */
#define GPIO_Pin_3                 ((uint16_t)0x0008)  /* Pin 3 selected */
#define GPIO_Pin_4                 ((uint16_t)0x0010)  /* Pin 4 selected */
#define GPIO_Pin_5                 ((uint16_t)0x0020)  /* Pin 5 selected */
#define GPIO_Pin_6                 ((uint16_t)0x0040)  /* Pin 6 selected */
#define GPIO_Pin_7                 ((uint16_t)0x0080)  /* Pin 7 selected */
#define GPIO_Pin_8                 ((uint16_t)0x0100)  /* Pin 8 selected */
#define GPIO_Pin_9                 ((uint16_t)0x0200)  /* Pin 9 selected */
#define GPIO_Pin_10                ((uint16_t)0x0400)  /* Pin 10 selected */
#define GPIO_Pin_11                ((uint16_t)0x0800)  /* Pin 11 selected */
#define GPIO_Pin_12                ((uint16_t)0x1000)  /* Pin 12 selected */
#define GPIO_Pin_13                ((uint16_t)0x2000)  /* Pin 13 selected */
#define GPIO_Pin_14                ((uint16_t)0x4000)  /* Pin 14 selected */
#define GPIO_Pin_15                ((uint16_t)0x8000)  /* Pin 15 selected */
#define GPIO_Pin_All               ((uint16_t)0xFFFF)  /* All pins selected */




#define GPIO_AF_USART1        ((uint8_t)0x07)  /* USART1 Alternate Function mapping  */


#define U32FROMU8(higher,high,low,lower)		((((uint32_t)(higher))<<24)|(((uint32_t)(high))<<16)|(((uint32_t)(low))<<8)|(uint32_t)(lower))

#endif /* BOOTLOADER_H_ */
