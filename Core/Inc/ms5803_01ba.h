/*
 * ms5803_01ba.h
 *
 *  Created on: 14 jul 2026
 *      Author: Mario
 */

#ifndef INC_MS5803_01BA_H_
#define INC_MS5803_01BA_H_

#include "main.h"

/*** I2C ADDRESSES ***/
#define MS5803_01BA_I2C_HIGH_ADDR (0x76 << 1)
#define MS5803_01BA_I2C_LOW_ADDR (0x77 << 1)

/*** COMMAND IDs ***/
#define MS5803_01BA_CMD_RST 0x1E
#define MS5803_01BA_ADC_CMD_READ 0x00

// Convert Commands D1 (Pressure)
#define MS5803_01BA_CONVERT_PRESSURE_OSR256 0x40 // Highest speed lowest precision
#define MS5803_01BA_CONVERT_PRESSURE_OSR512 0x42
#define MS5803_01BA_CONVERT_PRESSURE_OSR1024 0x44
#define MS5803_01BA_CONVERT_PRESSURE_OSR2048 0x46
#define MS5803_01BA_CONVERT_PRESSURE_OSR4096 0x48 // Lowest speed highest precision

// Convert Commands D2 (Temperature)
#define MS5803_01BA_CONVERT_TEMP_OSR256 0x50 // Highest speed lowest precision
#define MS5803_01BA_CONVERT_TEMP_OSR512 0x52
#define MS5803_01BA_CONVERT_TEMP_OSR1024 0x54
#define MS5803_01BA_CONVERT_TEMP_OSR2048 0x56
#define MS5803_01BA_CONVERT_TEMP_OSR4096 0x58 // Lowest speed highest precision

// Read PROM Commands
#define MS5803_01BA_PROM_READ0 0xA0
#define MS5803_01BA_PROM_READ1 0xA2
#define MS5803_01BA_PROM_READ2 0xA4
#define MS5803_01BA_PROM_READ3 0xA6
#define MS5803_01BA_PROM_READ4 0xA8
#define MS5803_01BA_PROM_READ5 0xAA
#define MS5803_01BA_PROM_READ6 0xAC
#define MS5803_01BA_PROM_READ7 0xAE

/** TYPEDEF AND STRUCTURES **/
typedef struct{
	uint16_t default_data;
	uint16_t c1;
	uint16_t c2;
	uint16_t c3;
	uint16_t c4;
	uint16_t c5;
	uint16_t c6;
	uint16_t checksum;
}ms5803_01ba_prom_t;

typedef struct{
	int32_t pressure;
	int32_t temp;
}MS5803_01BA_T;

/*** FUNCTION DEFINITIONS ***/
void MS5803_01BA_RST(I2C_HandleTypeDef *hi2c, uint16_t dev_address);
void MS5803_01BA_READ_PROM(I2C_HandleTypeDef *hi2c, uint16_t dev_address, ms5803_01ba_prom_t *prom_data);
void MS5803_01BA_INIT(I2C_HandleTypeDef *hi2c, uint16_t dev_address, ms5803_01ba_prom_t *prom_data);

uint32_t MS5803_01BA_READ_RAW(I2C_HandleTypeDef *hi2c, uint16_t dev_address, uint8_t cmd);
void MS5803_01BA_CALIBRATE_DATA(uint32_t raw_press, uint32_t raw_temp, ms5803_01ba_prom_t *prom, MS5803_01BA_T *ms5803_01ba);


#endif /* INC_MS5803_01BA_H_ */
