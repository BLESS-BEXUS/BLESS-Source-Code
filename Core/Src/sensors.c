/*
 * sensors.c
 *
 *  Created on: 17 mar 2026
 *      Author: Mariio
 */

#include "sensors.h"
#include "main.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi3;
extern UART_HandleTypeDef huart2;


INA219_t ina;




void Sensors_Init(){
	MAX31865_Init(&hspi3, CS_ETEMP_GPIO_Port, CS_ETEMP_Pin);
	if(INA219_INIT(&hi2c1, &ina, INA219_I2C_ADDRESS_GND_GND ,INA219_R100, 3.2) != 0){
		printf("ERROR INITIALIZATING INA");
	}

}

uint8_t readSensors(TelemetryPacket *packet){
	uint8_t success = 1;
	packet->sync = 0xB1E5;
	packet->error_flags = 0;

	// Internal Temperature
	float internalTemp = TMP102_Read_Temp(&hi2c1, TMP102_I2C_ADDRESS_GND);
	if(internalTemp > -100 && internalTemp < 150){
		packet->int_temp =(int16_t) (internalTemp * 100);
	}else{
		success = 0;
		packet->error_flags |= 0x01;
	}

	// External Temperature
	float externalTemp = MAX31865_Read_Temp(&hspi3, CS_ETEMP_GPIO_Port, CS_ETEMP_Pin);
	if(externalTemp != -318.65){
		packet->ext_temp =(int16_t) (externalTemp * 100);
	}
	else{
		success = 0;
		packet->error_flags |= 0x02;
	}

	// Humidity

	float rh = SHT4X_Measure_High(&hi2c1);

	if(rh != -4.44 || rh != -4.45){
		packet->ext_hum = (int16_t) (rh * 100);
	}else{
		success = 0;
		packet->error_flags |= 0x03;
	}

	// Current Consumption (mA)
	if(INA219_READ_CURRENT(&hi2c1, &ina)){
		packet->halow_curr = ina.current;
	}else{
		success = 0;
		packet->error_flags |= 0x10;
	}

	// Voltage measurement (mV)
	packet->halow_volt = 0;

	return success;
}
