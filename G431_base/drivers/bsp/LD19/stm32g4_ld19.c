/**
 *******************************************************************************
 * @file	stm32g4_ld19.c
 * @author	Samuel Poiraud
 * @date	Jun 13, 2024
 * @brief	Module pour récupérer les trames envoyé par le lidar ld19
 *******************************************************************************
 * @note  	Datasheet: https://www.elecrow.com/download/product/SLD06360F/LD19_Development%20Manual_V2.3.pdf
 */

#include "config.h"
#if USE_LD19
#include "QS_maths.h"
#include "stm32g4_ld19.h"
#include "stm32g4_ld19_display.h"
#include "stm32g4_uart.h"
#include "stdio.h"

#ifndef LD19_UART
	#define LD19_UART	UART1_ID
#endif


#define HEADER 				0x54
#define PI4096				12868
#define PI					3.141592654f
#define DISPLAY_ON_TFT		1



//We will not use directly this structure, but it is interesting to see how the frame is built.
/*typedef struct __attribute__((packed)) {	//This structure matches the frame format of the LD19!
	uint8_t header;
	uint8_t ver_len;
	uint16_t speed;
	uint16_t start_angle;
	LidarPointStructDef point[POINT_PER_PACK];
	uint16_t end_angle;
	uint16_t timestamp;
	uint8_t crc8;
}ld19_frame_t;
*/


#define VER_MASK	0xE0
#define LEN_MASK	0x1F

__attribute__((unused)) static uint8_t CalCRC8(uint8_t *p, uint8_t len);
static running_t LD19_parse(char c, ld19_frame_handler_t * f);
__attribute__((unused)) static void display_handler_infos(ld19_frame_handler_t * f);
static void LD19_rx_callback(void);

static ld19_frame_handler_t last_frame_handler;
static volatile bool flag_new_handler_available = false;
static volatile bool flag_error_receiving_handler = false;
static volatile bool flag_we_scratched_the_last_handler = false;


void LD19_init(void)
{
	BSP_UART_init(LD19_UART, 230400);
	BSP_UART_set_callback(LD19_UART, &LD19_rx_callback);
}

/*
 * This function is called when a new byte is received.
 * For each byte received, we get it and we try to parse it.
 */
void LD19_rx_callback(void)
{
	static ld19_frame_handler_t frame_handler;
	char c;
	while(BSP_UART_data_ready(LD19_UART))
	{
		c = BSP_UART_get_next_byte(LD19_UART);
		switch(LD19_parse(c, &frame_handler))
		{
			case END_OK:
				if(flag_new_handler_available == false){
					//flag_we_scratched_the_last_handler = true;	//the process_main seems to be too slow to handle the last handler
					last_frame_handler = frame_handler;	//we copy the received frame
					flag_new_handler_available = true;
				}
				break;
			case END_ERROR:
				flag_error_receiving_handler = true;
				break;
			case IN_PROGRESS:	//no break
			default:
				break;
		}
	}
}

void LD19_DEMO_process_main(void)
{
	if(flag_new_handler_available){
		if(DISPLAY_ON_TFT)
			BSP_LD19_display_on_tft(&last_frame_handler);
		else
			display_handler_infos(&last_frame_handler);
		flag_new_handler_available = false;
	}

	if(flag_error_receiving_handler){
		flag_error_receiving_handler = false;
		debug_printf("LD19 frame failure\n");
	}

	if (flag_we_scratched_the_last_handler){
		flag_we_scratched_the_last_handler = false;
		debug_printf("¤");
	}
}


static const uint8_t CrcTable[256] ={
0x00, 0x4d, 0x9a, 0xd7, 0x79, 0x34, 0xe3,
0xae, 0xf2, 0xbf, 0x68, 0x25, 0x8b, 0xc6, 0x11, 0x5c, 0xa9, 0xe4, 0x33,
0x7e, 0xd0, 0x9d, 0x4a, 0x07, 0x5b, 0x16, 0xc1, 0x8c, 0x22, 0x6f, 0xb8,
0xf5, 0x1f, 0x52, 0x85, 0xc8, 0x66, 0x2b, 0xfc, 0xb1, 0xed, 0xa0, 0x77,
0x3a, 0x94, 0xd9, 0x0e, 0x43, 0xb6, 0xfb, 0x2c, 0x61, 0xcf, 0x82, 0x55,
0x18, 0x44, 0x09, 0xde, 0x93, 0x3d, 0x70, 0xa7, 0xea, 0x3e, 0x73, 0xa4,
0xe9, 0x47, 0x0a, 0xdd, 0x90, 0xcc, 0x81, 0x56, 0x1b, 0xb5, 0xf8, 0x2f,
0x62, 0x97, 0xda, 0x0d, 0x40, 0xee, 0xa3, 0x74, 0x39, 0x65, 0x28, 0xff,
0xb2, 0x1c, 0x51, 0x86, 0xcb, 0x21, 0x6c, 0xbb, 0xf6, 0x58, 0x15, 0xc2,
0x8f, 0xd3, 0x9e, 0x49, 0x04, 0xaa, 0xe7, 0x30, 0x7d, 0x88, 0xc5, 0x12,
0x5f, 0xf1, 0xbc, 0x6b, 0x26, 0x7a, 0x37, 0xe0, 0xad, 0x03, 0x4e, 0x99,
0xd4, 0x7c, 0x31, 0xe6, 0xab, 0x05, 0x48, 0x9f, 0xd2, 0x8e, 0xc3, 0x14,
0x59, 0xf7, 0xba, 0x6d, 0x20, 0xd5, 0x98, 0x4f, 0x02, 0xac, 0xe1, 0x36,
0x7b, 0x27, 0x6a, 0xbd, 0xf0, 0x5e, 0x13, 0xc4, 0x89, 0x63, 0x2e, 0xf9,
0xb4, 0x1a, 0x57, 0x80, 0xcd, 0x91, 0xdc, 0x0b, 0x46, 0xe8, 0xa5, 0x72,
0x3f, 0xca, 0x87, 0x50, 0x1d, 0xb3, 0xfe, 0x29, 0x64, 0x38, 0x75, 0xa2,
0xef, 0x41, 0x0c, 0xdb, 0x96, 0x42, 0x0f, 0xd8, 0x95, 0x3b, 0x76, 0xa1,
0xec, 0xb0, 0xfd, 0x2a, 0x67, 0xc9, 0x84, 0x53, 0x1e, 0xeb, 0xa6, 0x71,
0x3c, 0x92, 0xdf, 0x08, 0x45, 0x19, 0x54, 0x83, 0xce, 0x60, 0x2d, 0xfa,
0xb7, 0x5d, 0x10, 0xc7, 0x8a, 0x24, 0x69, 0xbe, 0xf3, 0xaf, 0xe2, 0x35,
0x78, 0xd6, 0x9b, 0x4c, 0x01, 0xf4, 0xb9, 0x6e, 0x23, 0x8d, 0xc0, 0x17,
0x5a, 0x06, 0x4b, 0x9c, 0xd1, 0x7f, 0x32, 0xe5, 0xa8
};

/* Constitution de la trame :
 *
 * 		Header 	| Verlen	| Speed		| StartAngle	|  Data.......				| EndAngle	|Timestamp 	| CRC Check
 * 		0x54  	| 1 byte	| LSB MSB	| LSB MSB		| 3 bytes per sample		| LSB MSB	|LSB MSB	| 1 byte
 * 				| tttmmmmm  |
 * 				| 00101100 	| [°/sec]	| [0.01°]		| distance(16) intensity(8)	| [0.01°]	|[ms]%30000	|
 * 				type=1
 * 				nb bytes = 12
*/
static running_t LD19_parse(char c, ld19_frame_handler_t * f){
	running_t ret = IN_PROGRESS;

	if(f->state != CRC_CHECK){
		f->crc = CrcTable[(f->crc ^ c) & 0xff];
	}

	switch(f->state){
		case WAIT_HEADER:
			if(c == HEADER){
				f->state = VERLEN;
				f->crc = CrcTable[HEADER];
			}
			break;
		case VERLEN:
			if((c & VER_MASK) >> 5 != 1){
				ret = END_ERROR;
				f->state = WAIT_HEADER;
			}
			else
				f->state = SPEED_LSB;
			break;
		case SPEED_LSB:
			f->speed = c;
			f->state = SPEED_MSB;
			break;
		case SPEED_MSB:
			f->speed |= (uint16_t)(((uint16_t)c)<<8);
			f->state = START_ANGLE_LSB;
			break;
		case START_ANGLE_LSB:
			f->start_angle = c;
			f->state = START_ANGLE_MSB;
			break;
		case START_ANGLE_MSB:
			f->start_angle |= (uint16_t)(((uint16_t)c)<<8);
			//f->start_angle_rad = GEOMETRY_modulo_angle((int16_t)(((uint32_t)f->start_angle)*183/256)); //GEOMETRY_modulo_angle(-(int16_t)(((uint32_t)handler->start_angle)*183/256));	// °->rad4096 :    *0.01*PI4096/180
			f->state = DATA_DISTANCE_LSB;
			f->index_data = 0;
			break;
		case DATA_DISTANCE_LSB:
			f->point[f->index_data].distance = c;
			f->state = DATA_DISTANCE_MSB;
			break;
		case DATA_DISTANCE_MSB:
			f->point[f->index_data].distance |= (uint16_t)(((uint16_t)c)<<8);
			f->state = DATA_INTENSITY;
			break;
		case DATA_INTENSITY:
			f->point[f->index_data].intensity = c;
			f->index_data++;
			if(f->index_data >= POINT_PER_PACK)
				f->state = END_ANGLE_LSB;
			else
				f->state = DATA_DISTANCE_LSB;
			break;
		case END_ANGLE_LSB:
			f->end_angle = c;
			f->state = END_ANGLE_MSB;
			break;
		case END_ANGLE_MSB:
			f->end_angle |= (uint16_t)(((uint16_t)c)<<8);
			//f->end_angle_rad = GEOMETRY_modulo_angle((uint16_t)(((uint32_t)f->end_angle)*183/256)); //(float)((float)(f->end_angle)*0.01*PI/180);
			f->state = TIME_STAMP_LSB;
			break;
		case TIME_STAMP_LSB:
			f->timestamp = c;
			f->state = TIME_STAMP_MSB;
			break;
		case TIME_STAMP_MSB:
			f->timestamp |= (uint16_t)(((uint16_t)c)<<8);
			f->state = CRC_CHECK;
			break;
		case CRC_CHECK:
			if(c == f->crc){
				//float angle_step;
				//angle_step = (f->end_angle_rad - f->start_angle_rad);	//en principe, cet angle est positif, car le capteur augmente ses angles en sens horaire !

				if(f->start_angle < 18000 && f->start_angle > 00000){ // si start_angle appartient à l'intervale [10;170°]
					/*
					if(angle_step < 0) //on conditionne l'usage de GEOMETRY_modulo_angle_f au cas où le end_angle < start_angle
					{
						angle_step = GEOMETRY_modulo_angle(angle_step)/(12-1);
						for(uint8_t i = 0; i<POINT_PER_PACK; i++)
							f->computed_angle_rad[i] =  GEOMETRY_modulo_angle(f->start_angle_rad + angle_step*i);
					}
					else	//un peu crade, mais rentable d'un point de vue algorithmique
					{
						angle_step /=(12-1);
						for(uint8_t i = 0; i<POINT_PER_PACK; i++)
							f->computed_angle_rad[i] =  (f->start_angle_rad + angle_step*i);
					}
					 */
					ret = END_OK;
				}
			}
			else
				ret = END_ERROR;
			f->state = WAIT_HEADER;
			break;
		default:
			f->state = WAIT_HEADER;
			break;
	}
	return ret;
}

#define DISPLAY_ONE_FRAME_OUT_OF_X		1	//100
#define DISPLAY_WITH_DETAILS	0
__attribute__((unused)) static void display_handler_infos(ld19_frame_handler_t * f)
{
	static uint16_t previous_time = 0;
	uint16_t delta_t;
	delta_t = f->timestamp-previous_time;
	previous_time = f->timestamp;
	static uint16_t filter_this_frame = 0;
	filter_this_frame = (filter_this_frame + 1) % DISPLAY_ONE_FRAME_OUT_OF_X;
	if(!filter_this_frame)	//on peut remplacer ceci par if(1) si on veut voir toutes les trames
	{

		debug_printf("dt=%d [%.1f->%.1f] - ", delta_t, f->start_angle_rad, f->end_angle_rad);

		for(uint8_t i = 0; i<POINT_PER_PACK; i++)
		{
			#if DISPLAY_WITH_DETAILS
				debug_printf("%.1f:%d(%d) ", f->computed_angle_rad[i], f->point[i].distance, f->point[i].intensity);
			#else
				//if(handler->computed_angle_rad[i] < 0.5 )
				debug_printf("%d ", f->point[i].distance);
				//debug_printf("%d\n", handler->start_angle);
			#endif
		}
		debug_printf("\n");
	}
}

//fonction indicative donnée par le fournisseur. Ce calcul est repris au fil de la lecture des octets.
__attribute__((unused)) static uint8_t CalCRC8(uint8_t *p, uint8_t len){
	uint8_t crc = 0;
	uint16_t i;
	for (i = 0; i < len; i++){
		crc = CrcTable[(crc ^ *p++) & 0xff];
	}
	return crc;
}



#endif
