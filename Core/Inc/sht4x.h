/*
 * sht4x.h
 *
 *  Created on: 21 jun 2026
 *      Author: Mariio
 */

#ifndef INC_SHT4X_H_
#define INC_SHT4X_H_

#include "main.h"

/** I2C ADDRESS **/
#define SHT4X_I2C_ADDRESS (0x44 << 1)

/** Note that this sensor does not contain directly the register addresses public.
 * Instead, it use commands to perform it functions **/

/** SHT4X COMMANDS **/
#define SHT4X_CMD_MEASURE_HIGH 0xFD // Measure T and RH with high precision
#define SHT4X_CMD_MEASURE_MEDIUM 0xF6 // Measure T and RH with medium precision
#define SHT4X_CMD_MEASURE_LOW 0xE0 // Measure T and RH with low precision
#define SHT4X_CMD_READ_SERIAL 0x89 // Read serial
#define SHT4X_CMD_SOFT_RESET 0x94 // Soft reset
#define SHT4X_CMD_HEATER_HIGH_ONE 0x39 // Active heater high power for 1s and take a high precision measurement
#define SHT4X_CMD_HEATER_HIGH_DOT_ONE 0x32 // Active heater high power for 0.1s and take a high precision measurement
#define SHT4X_CMD_HEATER_MEDIUM_ONE 0x2F // Active heater medium power for 1s and take a high precision measurement
#define SHT4X_CMD_HEATER_MEDIUM_DOT_ONE 0x24 // Active heater medium power for 0.1s and take a high precision measurement
#define SHT4X_CMD_HEATER_LOW_ONE 0X1E // Active heater low power for 1s and take a high precision measurement
#define SHT4X_CMD_HEATER_LOW_DOT_ONE 0X15 // Active heater low power for 0.1s and take a high precision measurement

/** SHT4X FUNTIONS **/

float SHT4X_Measure_High(I2C_HandleTypeDef *hi2c);
float SHT4X_Measure_Medium(I2C_HandleTypeDef *hi2c);
float SHT4X_Measure_Low(I2C_HandleTypeDef *hi2c);

float SHT4X_Read_Serial(I2C_HandleTypeDef *hi2c);
float SHT4X_Soft_Reset(I2C_HandleTypeDef *hi2c);

float SHT4X_Heater_Measure_High_One(I2C_HandleTypeDef *hi2c);
float SHT4X_Heater_Measure_High_Dot_One(I2C_HandleTypeDef *hi2c);
float SHT4X_Heater_Measure_Medium_One(I2C_HandleTypeDef *hi2c);
float SHT4X_Heater_Measure_Medium_Dot_One(I2C_HandleTypeDef *hi2c);
float SHT4X_Heater_Measure_Low_One(I2C_HandleTypeDef *hi2c);
float SHT4X_Heater_Measure_Low_Dot_One(I2C_HandleTypeDef *hi2c);







#endif /* INC_SHT4X_H_ */
