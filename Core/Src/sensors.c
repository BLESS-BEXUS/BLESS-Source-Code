/*
 * sensors.c
 *
 *  Created on: 17 mar 2026
 *      Author: Mariio
 */

#include "sensors.h"
#include "main.h"
#include "cmsis_os.h"

#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi3;
extern UART_HandleTypeDef huart2;


INA219_t ina;
MS5803_01BA_T ms5803;
ms5803_01ba_prom_t ms5803_prom;
NPA730M_015A_T npa730;


void Sensors_Init(){
	MAX31865_Init(&hspi3, CS_ETEMP_GPIO_Port, CS_ETEMP_Pin);
	if(INA219_INIT(&hi2c1, &ina, INA219_I2C_ADDRESS_GND_GND ,INA219_R100, 3.2) != 0){
		printf("ERROR INITIALIZATING INA");
	}

	 MS5803_01BA_INIT(&hi2c1, MS5803_01BA_I2C_LOW_ADDR, &ms5803_prom);
}

uint8_t SensorsSelfCheck(){
	float internalTemp = TMP102_Read_Temp(&hi2c1, TMP102_I2C_ADDRESS_GND);
	osDelay(500);
	float externalTemp = MAX31865_Read_Temp(&hspi3, CS_ETEMP_GPIO_Port, CS_ETEMP_Pin);
	osDelay(500);
	float rh = SHT4X_Measure_High(&hi2c1);
	osDelay(500);

	printf("=====================================\r\n");
	printf("======= SELF CHECK INITIALIZED ======\r\n");
	printf("=====================================\r\n");

	if(internalTemp < -100 && internalTemp > 150){
		return 0;
	} else {
		printf("TMP102 Correctly Initialized\r\n");
	}

	if(externalTemp == -318.65){
		return 0;
	}else{
		printf("MAX31865 Correctly Initialized\r\n");
	}

	if(rh == -4.44 || rh == -4.45){
		return 0;
	}else{
		printf("SHT45 Correctly Initialized\r\n");
	}

	if(!INA219_READ_CURRENT(&hi2c1, &ina)){
		return 0;
	}else{
		printf("INA219 Correctly Initialized\r\n");
	}

	return 1;
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

	float sht_temp = 0.0f;
	float sht_hum = 0.0f;

	// Llamamos a la nueva función pasando la dirección de memoria (&) de las variables
	if (SHT4X_Measure_TempHum_High(&hi2c1, &sht_temp, &sht_hum) == 0) {
	    // Si la lectura ha ido bien, guardamos los datos en el paquete de telemetría.
	    // Como tu int_temp y ext_hum necesitan enteros, multiplicamos por 100.

	    packet->ext_hum = (uint16_t)(sht_hum * 100.0f);

	    // Aquí puedes asignar esta temperatura como tu temperatura de backup
	    // o sustituir directamente la que te estaba fallando:
	    packet->int_temp = (int16_t)(sht_temp * 100.0f);
	} else {
	    // Si falla el I2C del SHT45, encendemos un bit de error
	    packet->error_flags |= 0x02; // Por ejemplo, el bit 1 para el SHT45
	}

	// Current Consumption (mA)
	if(INA219_READ_CURRENT(&hi2c1, &ina)){
		packet->halow_curr = ina.current;
	}else{
		success = 0;
		packet->error_flags |= 0x10;
	}

	// Pressure
	int32_t raw_temp, raw_press;
	raw_press = MS5803_01BA_READ_RAW(&hi2c1, MS5803_01BA_I2C_LOW_ADDR, MS5803_01BA_CONVERT_PRESSURE_OSR4096);
	raw_temp = MS5803_01BA_READ_RAW(&hi2c1, MS5803_01BA_I2C_LOW_ADDR, MS5803_01BA_CONVERT_TEMP_OSR4096);

	MS5803_01BA_CALIBRATE_DATA(raw_press, raw_temp, &ms5803_prom, &ms5803);
	NPA730_I2C_FETCH_DATA_2BYTES(&hi2c1, NPA730_I2C_ADDRESS, &npa730);
	packet->env_press = npa730.pressure;



	return success;
}
