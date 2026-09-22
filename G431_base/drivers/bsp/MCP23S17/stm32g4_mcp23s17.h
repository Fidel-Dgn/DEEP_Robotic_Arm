/**
 *******************************************************************************
 * @file	stm32g4_mcp23s17.h
 * @author	vchav
 * @date	May 15, 2024
 * @brief	Module pour utiliser le GPIO Expander MCP23S17
 *******************************************************************************
 */
#ifndef BSP_MCP23S17_STM32G4_MCP23S17_H_
#define BSP_MCP23S17_STM32G4_MCP23S17_H_

#include "config.h"


typedef enum{
	MCP23S17_PORT_A,
	MCP23S17_PORT_B
} MCP23S17_port_e;

typedef enum{
	MCP23S17_DIRECTION_OUTPUT = 0,
	MCP23S17_DIRECTION_INPUT
}MCP23S17_direction_e;

typedef enum{
	MCP23S17_PULL_UP_STATE_LOW = 0,
	MCP23S17_PULL_UP_STATE_HIGH
}MCP23S17_pullUpState_e;

typedef enum{
	MCP23S17_PIN_STATE_LOW = 0,
	MCP23S17_PIN_STATE_HIGH
}MCP23S17_pinState_e;



#if USE_MCP23S17
#include "stm32g4_utils.h"

#ifndef MCP23S17_SPI
	#define MCP23S17_SPI           	SPI1
	#define MCP23S17_SPI_PORT		GPIOA
	#define MCP23S17_SPI_MISO_PIN	GPIO_PIN_6
	#define MCP23S17_SPI_MOSI_PIN	GPIO_PIN_7
	#define MCP23S17_SPI_SCK_PIN	GPIO_PIN_5
#endif


#ifndef MCP23S17_CS_PIN
#define MCP23S17_CS_PORT       GPIOA
#define MCP23S17_CS_PIN        GPIO_PIN_12
#endif

#ifndef MCP23S17_RST_PIN
#define MCP23S17_RST_PORT       GPIOB
#define MCP23S17_RST_PIN        GPIO_PIN_0
#endif


/*
 * Les registres sont adress�s avec IOCON.BANK = 0 (les adresses se suivent)
 * Les adresses d�finies ci dessous correspondent en r�alit� aux adresses des registres du port A.
 * Mais comme elles ce suivent avec les adresses des registres du port B, on peut retrouver les adresses des registres B avec les A.
 * exemple:
 * 			- MCP23S17_IODIRA = 0x00 et MCP23S17_IODIRB = 0x01 ( = MCP23S17_IODIRA + 1)
 * 			- MCP23S17_IPOLA = 0x02 et MCP23S17_IPOL = 0x03 ( = MCP23S17_IPOLA + 1)
 * 			- ...
 * Dans le code, le role du +1 est jou� par "+ MCP23S17_PORT_B" dans l'enum MCP23S17_port_e ci dessous
*/
#define MCP23S17_IODIR 		0x00
#define MCP23S17_IPOL 		0x02
#define MCP23S17_GPINTEN 	0x04
#define MCP23S17_DEFVAL 	0x06
#define MCP23S17_INTCON 	0x08
#define MCP23S17_IOCON 		0x0A
#define MCP23S17_GPPU 		0x0C
#define MCP23S17_INTF		0x0E
#define MCP23S17_INTCAP		0x10
#define MCP23S17_GPIO 		0x12
#define MCP23S17_OLAT      	0x14

// Le bit 0 n'est pas utilis�
#define IOCON_POLARITY_BIT 	0x02 // (bit 1 --> 0000 0010)
#define IOCON_ODR_BIT 		0x04 // (bit 2 --> 0000 0100)
#define IOCON_HAEN_BIT		0x08 // ...
#define IOCON_DISSLW_BIT	0x10
#define IOCON_SEQOP_BIT		0x20
#define IOCON_MIRROR_BIT 	0x40 // ...
#define IOCON_BANK_BIT		0x80 // (bit 7 --> 1000 0000)


typedef enum{
	ACTIVE_LOW = 0,
	ACTIVE_HIGH
} MCP23S17_polarity_e;



typedef enum{
	MCP23S17_PIN_0	= 0b00000001,
	MCP23S17_PIN_1	= 0b00000010,
	MCP23S17_PIN_2	= 0b00000100,
	MCP23S17_PIN_3	= 0b00001000,
	MCP23S17_PIN_4	= 0b00010000,
	MCP23S17_PIN_5	= 0b00100000,
	MCP23S17_PIN_6	= 0b01000000,
	MCP23S17_PIN_7	= 0b10000000
}MCP23S17_pin_e;




typedef enum{
	MCP23S17_IT_ON_CHANGE = 0, //Type de comparaison 1: on d�clenche une IT lorsque la valeur du GPIO change
	MCP23S17_IT_ON_REF	  	  //Type de comparaison 2: on d�clanche une IT lorsque la valeur du GPIO diffère de sa valeur de r�f�rence inscrite dans le registre DEFVAL
}MCP23S17_itControlType_e;

typedef enum{
	MCP23S17_IT_DISABLE = 0,
	MCP23S17_IT_ENABLE
}MCP23S17_itState_e;


void BSP_MCP23S17_init(void);
void BSP_MCP23S17_demo(MCP23S17_direction_e direction);

void BSP_MCP23S17_setGPIODirection(MCP23S17_port_e port, MCP23S17_pin_e pin, MCP23S17_direction_e direction);

void BSP_MCP23S17_setPullUp_onPin(MCP23S17_port_e port, MCP23S17_pin_e pin, MCP23S17_pullUpState_e state);
void BSP_MCP23S17_setPullUp_onPort(MCP23S17_port_e port, MCP23S17_pullUpState_e state);

void BSP_MCP23S17_setITState_onPin(MCP23S17_port_e port, MCP23S17_pin_e pin, MCP23S17_itState_e state);
void BSP_MCP23S17_setITState_onPort(MCP23S17_port_e port, MCP23S17_itState_e state);

void BSP_MCP23S17_setMirrorIT(bool mirror);

void BSP_MCP23S17_setITPolarity(MCP23S17_polarity_e polarity);

void BSP_MCP23S17_setDefaultValue_onPin(MCP23S17_port_e port, MCP23S17_pin_e pin, MCP23S17_pinState_e state);
void BSP_MCP23S17_setDefaultValue_onPort(MCP23S17_port_e port, MCP23S17_pinState_e state);

void BSP_MCP23S17_setITControl_onPin(MCP23S17_port_e port, MCP23S17_pin_e pin, MCP23S17_itControlType_e type);
void BSP_MCP23S17_setITControl_onPort(MCP23S17_port_e port, MCP23S17_itControlType_e type);

void BSP_MCP23S17_writeGPIO(MCP23S17_port_e port, MCP23S17_pin_e pin, MCP23S17_pinState_e state);
uint8_t BSP_MCP23S17_readGPIO(MCP23S17_port_e port);

#endif
#endif /* BSP_MCP23S17_STM32G4_MCP23S17_H_ */
