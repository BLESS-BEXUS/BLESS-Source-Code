/*
 * npa730n015a.c
 *
 *  Created on: 14 jul 2026
 *      Author: Mario
 */

#include "npa730n015a.h"

HAL_StatusTypeDef NPA730_I2C_FETCH_DATA_2BYTES(I2C_HandleTypeDef *hi2c,
		uint8_t dev_address, NPA730M_015A_T *npa730){
	uint8_t data[2];
	uint16_t raw_pressure;
	HAL_StatusTypeDef status = HAL_I2C_Master_Receive(hi2c, dev_address,
			data, 2, 100);

	if(status != HAL_OK){
		npa730->status = 3U;
		npa730->pressure = -1.0f;
		return status;
	}

	raw_pressure = (data[0] << 8) | (data[1]);
	npa730->status = raw_pressure >> 14;

	raw_pressure = raw_pressure & 0x3FFF;
	npa730->pressure = ((float)(raw_pressure - 1638) / 13107.0f) * 15.0f;
	return HAL_OK;
}

