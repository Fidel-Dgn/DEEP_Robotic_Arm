/**
 *******************************************************************************
 * @file	stm32g4_ld19.h
 * @author	Samuel Poiraud
 * @date	Jun 13, 2024
 * @brief	Module pour récupérer les trames envoyé par le lidar ld19
 *******************************************************************************
 */
#ifndef BSP_LD19_STM32G4_LD19_H_
#define BSP_LD19_STM32G4_LD19_H_
#if USE_LD19
#define POINT_PER_PACK 12

void LD19_init(void);

void LD19_DEMO_process_main(void);

typedef struct
{
	int32_t x;
	int32_t y;
	int32_t theta;
}pos_t;

//Un "point" est un couple de distance et d'intensité
typedef struct __attribute__((packed)) {
	uint16_t distance;
	uint8_t intensity;
} ld19_point_t;

//Cette enum miste les différentes étapes du traitement des données reçus sur l'UART
typedef enum{
	WAIT_HEADER,
	VERLEN,
	SPEED_LSB,
	SPEED_MSB,
	START_ANGLE_LSB,
	START_ANGLE_MSB,
	DATA_DISTANCE_LSB,
	DATA_DISTANCE_MSB,
	DATA_INTENSITY,
	END_ANGLE_LSB,
	END_ANGLE_MSB,
	TIME_STAMP_LSB,
	TIME_STAMP_MSB,
	CRC_CHECK
}parse_state_e;

//We will store in this structure the information of the frame and related to the parsing.
typedef struct
{
	uint16_t timestamp;
	ld19_point_t point[POINT_PER_PACK];
	uint16_t start_angle;
	uint16_t end_angle;
	uint16_t speed;
	parse_state_e state;
	uint8_t index_data;
	uint8_t crc;
	uint16_t start_angle_rad;
	uint16_t end_angle_rad;
	uint16_t computed_angle_rad[POINT_PER_PACK];
}ld19_frame_handler_t;

#endif
#endif /* BSP_LD19_STM32G4_LD19_H_ */
