/**
 *******************************************************************************
 * @file	stm32g4_flash.c
 * @author	vchav
 * @author  Samuel Poiraud
 * @date	Jun 11, 2024
 * @brief	Adaptation du module créé par Samuel Poiraud pour la stm32f103rbt6
 *******************************************************************************
 */
#include "stm32g4_flash.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_flash_ex.h"
#include <stdio.h>
#include <assert.h>

#define BASE_ADDRESS					0x0801F800		//adresse du début de la dernière page (2kBytes)
#define PAGE_USED_FOR_THIS_MODULE    	63
#define	SIZE_SECTOR_IN_BYTES			(2048)
#define SIZE_SECTOR_IN_DOUBLEWORDS		(SIZE_SECTOR_IN_BYTES/8)


static void FLASH_write_doubleword(uint32_t index, uint64_t data);
static void FLASH_keeping_everything_else(uint32_t index);
static void FLASH_erase(void);
extern void FLASH_PageErase(uint32_t PageAddress, uint32_t Banks);


/**
 * @brief	Fonction de démo permettant de se familiariser avec les fonctions de ce module logiciel.
 * @brief	Cette fonction consulte et incrémente le nombre inscrit dans la première case. Puis, elle lance un affichage du contenu complet.
 * @pre		ATTENTION : ne pas appeler cette fonction en tâche de fond. Risque d'endommager la flash en cas d'écritures trop nombreuses.
 */
void FLASH_demo(void)
{
	uint64_t current_doubleword;
	current_doubleword = BSP_FLASH_read_doubleword(0);
    uint32_t low = (uint32_t)(current_doubleword & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(current_doubleword >> 32);
	printf("double-mot pr\xE9sent dans la case 0 : %lu;%lu = 0x%lx;0x%lx\n", high, low,  high, low);
	current_doubleword++;
	BSP_FLASH_set_doubleword(0, current_doubleword);
	current_doubleword = BSP_FLASH_read_doubleword(0);
	low = (uint32_t)(current_doubleword & 0xFFFFFFFF);
	high = (uint32_t)(current_doubleword >> 32);
	printf("double-mot pr\xE9sent dans la case 0 apr\xE8s incr\xE9mentation : %lu;%lu = 0x%lx;0x%lx\n", high, low,  high, low);
	BSP_FLASH_dump();
}




/**
 * @brief	Enregistre une donnée dans la case souhaitée, sans toucher aux autres cases
 * @param  	index: Numéro de la case (de 0 à 255).
 * @post  	ATTENTION : si la case est déjà occupée par une donnée différente de 0xFFFFFFFF (valeur par défaut après effacement), une sauvegarde complète du secteur est faite, puis un effacement, puis une restitution !
 * @post  	le temps d'exécution de cette fonction peut nettement varier !
 * @pre		//ATTENTION : ne pas appeler cette fonction trop fréquemment. Risque d'endommager la flash en cas d'écritures trop nombreuses. (>10000 sur le cycle de vie complet du produit)
 */
void BSP_FLASH_set_doubleword(uint32_t index, uint64_t data)
{
	uint64_t current_doubleword;
	assert(index < SIZE_SECTOR_IN_DOUBLEWORDS);
	current_doubleword = BSP_FLASH_read_doubleword(index);
	if((current_doubleword & data) != data)	//il n'est pas possible d'écrire le mot sans être pollué par des zéros qui seraient déjà écrit ici
		FLASH_keeping_everything_else(index);

	FLASH_write_doubleword(index, data);
}


/**
 * @brief	Lit une donnee situee dans la case souhaitee.
 * @param	index: Numero de la case (de 0 a SIZE_SECTOR_IN_WORDS-1).
 */
uint64_t BSP_FLASH_read_doubleword(uint32_t index)
{
	assert(index < SIZE_SECTOR_IN_DOUBLEWORDS);

	uint64_t * p;
	p = (uint64_t *)(BASE_ADDRESS + 8*index);

	return (uint64_t)(*p);
}


/**
 * @brief Cette fonction affiche les SIZE_SECTOR données (32 bits) disponibles dans le dernier secteur de la FLASH
 */
void BSP_FLASH_dump(void){
	uint32_t index;
	uint64_t v;
	printf("Affichage des %d donnees (64 bits) disponibles dans le dernier secteur de la FLASH\n", SIZE_SECTOR_IN_DOUBLEWORDS);
	for(index = 0; index<SIZE_SECTOR_IN_DOUBLEWORDS; index++)
	{
		v = BSP_FLASH_read_doubleword(index);
		uint32_t low = (uint32_t)(v & 0xFFFFFFFF);
		uint32_t high = (uint32_t)(v >> 32);
		printf("@%03ld : 0x%08lx%08lx = %ld;%ld\n", index, high, low, high, low);
	}
}


static void FLASH_keeping_everything_else(uint32_t index)
{
	uint64_t saved_values[SIZE_SECTOR_IN_DOUBLEWORDS];
	uint32_t i;
	assert(index < SIZE_SECTOR_IN_DOUBLEWORDS);
	for(i=0; i<SIZE_SECTOR_IN_DOUBLEWORDS; i++)
		saved_values[i] = BSP_FLASH_read_doubleword(i);
	FLASH_erase();
	for(i=0; i<SIZE_SECTOR_IN_DOUBLEWORDS; i++)
	{
		if(i!=index && saved_values[i]!=(uint64_t)0xFFFFFFFFFFFFFFFF)
			FLASH_write_doubleword(i, saved_values[i]);	//on réécrit tout sauf le mot
	}
}

static void FLASH_erase(void)
{
	HAL_FLASH_Unlock();
	FLASH_PageErase(PAGE_USED_FOR_THIS_MODULE, FLASH_BANK_1);
	 /* Wait for last operation to be completed */
	FLASH_WaitForLastOperation((uint32_t)FLASH_TIMEOUT_VALUE);

	/* If the erase operation is completed, disable the PER Bit */
	CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
	HAL_FLASH_Lock();
}


static void FLASH_write_doubleword(uint32_t index, uint64_t data)
{
	assert(index < SIZE_SECTOR_IN_DOUBLEWORDS);
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, BASE_ADDRESS+8*index, (uint64_t)(data));
	HAL_FLASH_Lock();
}
