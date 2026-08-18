/*
 * ms5803_01ba.c
 *
 *  Created on: 14 jul 2026
 *      Author: Mario
 */


#include "ms5803_01ba.h"

void MS5803_01BA_RST(I2C_HandleTypeDef *hi2c, uint16_t dev_address){
	uint8_t cmd = MS5803_01BA_CMD_RST;
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_Delay(3);
	//osDelay(3);
}

void MS5803_01BA_READ_PROM(I2C_HandleTypeDef *hi2c, uint16_t dev_address, ms5803_01ba_prom_t *prom_data){
	uint8_t data[2];
	uint8_t cmd = MS5803_01BA_PROM_READ0;
	// Read the default manufacturer data
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_I2C_Master_Receive(hi2c, dev_address, data, 2, 100);
	prom_data->default_data = (data[0] << 8) | data[1];

	// Read the first calibration point
	cmd = MS5803_01BA_PROM_READ1;
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_I2C_Master_Receive(hi2c, dev_address, data, 2, 100);
	prom_data->c1 = (data[0] << 8) | data[1];

	// Read the second calibration point
	cmd = MS5803_01BA_PROM_READ2;
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_I2C_Master_Receive(hi2c, dev_address, data, 2, 100);
	prom_data->c2 = (data[0] << 8) | data[1];

	// Read the third calibration point
	cmd = MS5803_01BA_PROM_READ3;
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_I2C_Master_Receive(hi2c, dev_address, data, 2, 100);
	prom_data->c3 = (data[0] << 8) | data[1];

	// Read the fourth calibration point
	cmd = MS5803_01BA_PROM_READ4;
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_I2C_Master_Receive(hi2c, dev_address, data, 2, 100);
	prom_data->c4 = (data[0] << 8) | data[1];

	// Read the fifth calibration point
	cmd = MS5803_01BA_PROM_READ5;
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_I2C_Master_Receive(hi2c, dev_address, data, 2, 100);
	prom_data->c5 = (data[0] << 8) | data[1];

	// Read the sixth calibration point
	cmd = MS5803_01BA_PROM_READ6;
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_I2C_Master_Receive(hi2c, dev_address, data, 2, 100);
	prom_data->c6 = (data[0] << 8) | data[1];

	// Read the checksum
	cmd = MS5803_01BA_PROM_READ7;
	HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	HAL_I2C_Master_Receive(hi2c, dev_address, data, 2, 100);
	prom_data->checksum = (data[0] << 8) | data[1];
}

void MS5803_01BA_INIT(I2C_HandleTypeDef *hi2c, uint16_t dev_address, ms5803_01ba_prom_t *prom_data){
	MS5803_01BA_RST(hi2c, dev_address);
	MS5803_01BA_READ_PROM(hi2c, dev_address, prom_data);
}

uint32_t MS5803_01BA_READ_RAW(I2C_HandleTypeDef *hi2c, uint16_t dev_address, uint8_t cmd){
	uint8_t data[3];
	uint8_t rx_cmd = MS5803_01BA_ADC_CMD_READ;
	HAL_StatusTypeDef status;

	// Send conversion command
	status = HAL_I2C_Master_Transmit(hi2c, dev_address, &cmd, 1, 100);
	if(status != HAL_OK){
		return 0;
	}
	HAL_Delay(10);
	//osDelay(10);

	// Read the final raw value
	status = HAL_I2C_Master_Transmit(hi2c, dev_address, &rx_cmd, 1, 100);
	if(status != HAL_OK){
		return 0;
	}

	HAL_I2C_Master_Receive(hi2c, dev_address, data, 3, 100);

	return (data[0] << 16) | (data[1] << 8) | data[2];
}

void MS5803_01BA_CALIBRATE_DATA(uint32_t raw_press, uint32_t raw_temp, ms5803_01ba_prom_t *prom, MS5803_01BA_T *ms5803_01ba){
	/** First Order Calibration**/
	// Calculate temperature
	int32_t dT = raw_temp - ((int32_t)prom->c5 << 8);
	int32_t temp = 2000 + (int32_t)(((int64_t)dT * prom->c6) >> 23);
	// Calculate temperature compensated pressure
	int64_t offset = ((int64_t)prom->c2 << 16) + (((int64_t)prom->c4 * dT) >> 7);
	int64_t sensivity = ((int64_t)prom->c1 << 15) + (((int64_t)prom->c3 * dT) >> 8);

	/** Second Order Calibration**/
	int32_t t2 = 0;
	int64_t off2 = 0;
	int64_t sens2 = 0;

	if(temp < 2000){
		t2 = (int32_t)(((int64_t)dT * dT) >> 31);
		off2 = 3 * ((int64_t)(temp - 2000) * (temp - 2000));
		sens2 = (7 * ((int64_t)(temp - 2000) * (temp - 2000))) >> 3;
		if(temp < -1500){
			sens2 = sens2 + 2 * ((int64_t)(temp + 1500) * (temp + 1500));
		}

	}else if(temp >= 4500){
			sens2 = sens2 - (((int64_t)(temp - 4500) * (temp - 4500)) >> 3);
	}

	temp = temp - t2;
	offset = offset - off2;
	sensivity = sensivity - sens2;

	int32_t pressure = (int32_t)(((((int64_t)raw_press * sensivity) >> 21) - offset) >> 15);

	ms5803_01ba->pressure = pressure;
	ms5803_01ba->temp = temp;
}


