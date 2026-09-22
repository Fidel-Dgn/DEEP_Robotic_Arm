/**
 *******************************************************************************
 * @file	stm32g4_dac.h
 * @author	vchav
 * @date	Jun 7, 2024
 * @brief	Module pour utiliser le DAC (Digital to Analog Converter) sur cible stm32g4
 *******************************************************************************
 */
#ifndef STM32G4_DAC_H_
#define STM32G4_DAC_H_

#include "config.h"
#if USE_DAC
#include "stm32g4_utils.h"

typedef enum{
	DAC1_OUT1,
	DAC1_OUT2
}dac_out_e;

typedef enum{
	DAC_MODE_SAMPLE_AND_HOLD,
	DAC_MODE_NORMAL
}dac_mode_e;
void BSP_DAC_demo_without_dma();
void BSP_DAC_demo_with_dma() ;
void BSP_DAC_Init(dac_out_e out, dac_mode_e mode, bool with_dma);
void BSP_DAC_Start_without_dma(dac_out_e outx);
void BSP_DAC_Start_dma(dac_out_e outx, uint32_t* dac_buffer, uint32_t size);
bool BSP_DAC_Set_value(dac_out_e outx, uint32_t value);

#endif
#endif /* STM32G4_DAC_H_ */
