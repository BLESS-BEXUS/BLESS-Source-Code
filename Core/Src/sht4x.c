/*
 * sht4x.c
 *
 *  Created on: 21 jun 2026
 *      Author: Mariio
 */

#include "sht4x.h"
#include "cmsis_os.h"

float SHT4X_Measure_High(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_MEASURE_HIGH;

	HAL_StatusTypeDef status;
	uint16_t rh_raw, temp_raw;

	float rh, temp;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}

	// This project just use the humidity data, the temperature bytes (0, 1 and 2 of "data) are nor used.

	rh_raw = (data[3] << 8) | data[4];
	temp_raw = (data[0] << 8) | data[1];

	temp = -45.0f + 175.0f * ((float)temp_raw/65535.0f);
	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}

float SHT4X_Measure_Medium(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_MEASURE_MEDIUM;

	HAL_StatusTypeDef status;
	uint16_t rh_raw;
	float rh;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}

	// This project just use the humidity data, the temperature bytes (0, 1 and 2 of "data) are nor used.

	rh_raw = (data[3] << 8) | data[4];

	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}

float SHT4X_Measure_Low(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_MEASURE_LOW;

	HAL_StatusTypeDef status;
	uint16_t rh_raw;
	float rh;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}

	// This project just use the humidity data, the temperature bytes (0, 1 and 2 of "data) are nor used.

	rh_raw = (data[3] << 8) | data[4];

	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}

float SHT4X_Read_Serial(I2C_HandleTypeDef *hi2c);
float SHT4X_Soft_Reset(I2C_HandleTypeDef *hi2c);

float SHT4X_Heater_Measure_High_One(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_HEATER_HIGH_ONE;

	HAL_StatusTypeDef status;
	uint16_t rh_raw;
	float rh;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}


	rh_raw = (data[3] << 8) | data[4];

	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}
float SHT4X_Heater_Measure_High_Dot_One(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_HEATER_HIGH_DOT_ONE;

	HAL_StatusTypeDef status;
	uint16_t rh_raw;
	float rh;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}

	// This project just use the humidity data, the temperature bytes (0, 1 and 2 of "data) are nor used.

	rh_raw = (data[3] << 8) | data[4];

	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}

float SHT4X_Heater_Measure_Medium_One(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_HEATER_MEDIUM_ONE;

	HAL_StatusTypeDef status;
	uint16_t rh_raw;
	float rh;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}

	// This project just use the humidity data, the temperature bytes (0, 1 and 2 of "data) are nor used.

	rh_raw = (data[3] << 8) | data[4];

	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}
float SHT4X_Heater_Measure_Medium_Dot_One(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_HEATER_MEDIUM_DOT_ONE;

	HAL_StatusTypeDef status;
	uint16_t rh_raw;
	float rh;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}

	// This project just use the humidity data, the temperature bytes (0, 1 and 2 of "data) are nor used.

	rh_raw = (data[3] << 8) | data[4];

	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}
float SHT4X_Heater_Measure_Low_One(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_HEATER_LOW_ONE;

	HAL_StatusTypeDef status;
	uint16_t rh_raw;
	float rh;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}

	// This project just use the humidity data, the temperature bytes (0, 1 and 2 of "data) are nor used.

	rh_raw = (data[3] << 8) | data[4];

	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}
float SHT4X_Heater_Measure_Low_Dot_One(I2C_HandleTypeDef *hi2c){
	uint8_t data[6];
	uint8_t cmd = SHT4X_CMD_HEATER_LOW_DOT_ONE;

	HAL_StatusTypeDef status;
	uint16_t rh_raw;
	float rh;
	status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);

	if(status != HAL_OK){
		return -4.44;
	}

	osDelay(10);

	status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
	if(status != HAL_OK){
		return -4.45;
	}


	rh_raw = (data[3] << 8) | data[4];

	rh = -6.0f + 125.0f * ((float)rh_raw/65535.0f);

	if (rh > 100){
		return 100;
	}
	if (rh < 0){
		return 0;
	}

	// Return the %RH
	return rh;
}



uint8_t SHT4X_Measure_TempHum_High(I2C_HandleTypeDef *hi2c, float *out_temp, float *out_hum) {
    uint8_t data[6];
    uint8_t cmd = SHT4X_CMD_MEASURE_HIGH;

    HAL_StatusTypeDef status;
    uint16_t t_raw, rh_raw;

    status = HAL_I2C_Master_Transmit(hi2c, SHT4X_I2C_ADDRESS, &cmd, 1, 100);
    if(status != HAL_OK) return 1;

    osDelay(10);

    status = HAL_I2C_Master_Receive(hi2c, SHT4X_I2C_ADDRESS, data, 6, 100);
    if(status != HAL_OK) return 1;

    t_raw = (data[0] << 8) | data[1];
    *out_temp = -45.0f + 175.0f * ((float)t_raw / 65535.0f);

    rh_raw = (data[3] << 8) | data[4];
    *out_hum = -6.0f + 125.0f * ((float)rh_raw / 65535.0f);

    if (*out_hum > 100.0f) *out_hum = 100.0f;
    if (*out_hum < 0.0f) *out_hum = 0.0f;

    return 0;
}

