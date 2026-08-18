/*
 * npa730n015a.h
 *
 *  Created on: 14 jul 2026
 *      Author: Mario
 */

#ifndef INC_NPA730N015A_H_
#define INC_NPA730N015A_H_

#include "main.h"

/** I2C ADDRESSES **/
#define NPA730_I2C_ADDRESS (0x28 << 1)

/** TYPEDEF AND STRUCTS **/
typedef struct{
	float pressure;
	uint16_t temp;
	uint8_t status;
}NPA730M_015A_T;

/** FUNCTIONS DEFINITION **/
void NPA730_I2C_FETCH_DATA_2BYTES(I2C_HandleTypeDef *hi2c, uint8_t dev_address, NPA730M_015A_T *npa730);


#endif /* INC_NPA730N015A_H_ */
