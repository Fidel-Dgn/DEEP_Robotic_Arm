/**
 *******************************************************************************
 * @file 	stm32g4_mcp23017.h
 * @author 	vchav
 * @date 	May 6, 2024
 * @brief	Module pour utiliser le GPIO Expander MCP23017.
 *******************************************************************************
 */

#ifndef BSP_MCP23017_STM32G4_MCP23017_H_
#define BSP_MCP23017_STM32G4_MCP23017_H_

#include "config.h"
#if USE_MCP23017
#include "stm32g4_i2c.h"
#include "stm32g4_utils.h"

#ifndef MCP23017_NB_IC
	#define MCP23017_NB_IC	2
#endif

typedef uint8_t MCP23017_address_t;

#define MCP23017_ID_ERROR		0xFF
typedef uint8_t MCP23017_id_t;

typedef enum{
	MCP23017_POLARITY_ACTIVE_LOW,
	MCP23017_POLARITY_ACTIVE_HIGH
}MCP23017_polarity_e;

typedef enum{
	MCP23017_PORT_A,
	MCP23017_PORT_B
}MCP23017_port_e;

typedef enum{
	MCP23017_PIN_0	= 0b00000001,
	MCP23017_PIN_1	= 0b00000010,
	MCP23017_PIN_2	= 0b00000100,
	MCP23017_PIN_3	= 0b00001000,
	MCP23017_PIN_4	= 0b00010000,
	MCP23017_PIN_5	= 0b00100000,
	MCP23017_PIN_6	= 0b01000000,
	MCP23017_PIN_7	= 0b10000000
}MCP23017_pin_e;

typedef enum{
	MCP23017_PIN_STATE_LOW,
	MCP23017_PIN_STATE_HIGH
}MCP23017_pinState_e;

typedef enum{
	MCP23017_PULL_UP_STATE_LOW,
	MCP23017_PULL_UP_STATE_HIGH
}MCP23017_pullUpState_e;

typedef enum{
	MCP23017_DIRECTION_INPUT,
	MCP23017_DIRECTION_OUTPUT
}MCP23017_direction_e;

bool MCP23017_init();

MCP23017_id_t MCP23017_add(I2C_TypeDef* I2Cx, MCP23017_address_t address);

bool MCP23017_setIO(MCP23017_id_t id, MCP23017_port_e port, MCP23017_pin_e pin, MCP23017_direction_e direction);
bool MCP23017_getIO(MCP23017_id_t id, MCP23017_port_e port, MCP23017_pin_e pin, MCP23017_direction_e * direction);

bool MCP23017_setGPIO(MCP23017_id_t id, MCP23017_port_e port, MCP23017_pin_e pin, MCP23017_pinState_e state);
bool MCP23017_getGPIO(MCP23017_id_t id, MCP23017_port_e port, MCP23017_pin_e pin, MCP23017_pinState_e * state);

bool MCP23017_setPullUp(MCP23017_id_t id, MCP23017_port_e port, MCP23017_pin_e pin, MCP23017_pullUpState_e state);
bool MCP23017_getPullUp(MCP23017_id_t id, MCP23017_port_e port, MCP23017_pin_e pin, MCP23017_pullUpState_e * state);

#endif
#endif /* BSP_MCP23017_STM32G4_MCP23017_H_ */
