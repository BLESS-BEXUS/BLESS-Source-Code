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
extern osMutexId_t mtxI2C1Handle;


INA219_t ina;
MS5803_01BA_T ms5803;
ms5803_01ba_prom_t ms5803_prom;
NPA730M_015A_T npa730;

#define PSI_TO_PA 6894.7573f
#define SENSOR_I2C_READY_TRIALS 2U
#define SENSOR_I2C_READY_TIMEOUT_MS 50U

static void Sensors_I2C_Lock(void){
	if(osMutexAcquire(mtxI2C1Handle, osWaitForever) != osOK){
		Error_Handler();
	}
}

static void Sensors_I2C_Unlock(void){
	osMutexRelease(mtxI2C1Handle);
}

static void Sensors_I2C_RecoverBus(void){
	GPIO_InitTypeDef gpio = {0};
	uint32_t error;
	HAL_I2C_StateTypeDef state;

	Sensors_I2C_Lock();
	error = HAL_I2C_GetError(&hi2c1);
	state = HAL_I2C_GetState(&hi2c1);

	(void)HAL_I2C_DeInit(&hi2c1);

	/* Temporarily control the bus as open-drain GPIO. Nine SCL pulses release
	 * a slave left in the middle of a byte; the final sequence generates STOP. */
	gpio.Pin = SCL_SENSORS_Pin | SDA_SENSORS_Pin;
	gpio.Mode = GPIO_MODE_OUTPUT_OD;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &gpio);
	HAL_GPIO_WritePin(GPIOB, SCL_SENSORS_Pin | SDA_SENSORS_Pin, GPIO_PIN_SET);
	HAL_Delay(1);

	for(uint8_t pulse = 0U; pulse < 9U; ++pulse){
		HAL_GPIO_WritePin(SCL_SENSORS_GPIO_Port, SCL_SENSORS_Pin, GPIO_PIN_RESET);
		HAL_Delay(1);
		HAL_GPIO_WritePin(SCL_SENSORS_GPIO_Port, SCL_SENSORS_Pin, GPIO_PIN_SET);
		HAL_Delay(1);
	}

	HAL_GPIO_WritePin(SCL_SENSORS_GPIO_Port, SCL_SENSORS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SDA_SENSORS_GPIO_Port, SDA_SENSORS_Pin, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(SCL_SENSORS_GPIO_Port, SCL_SENSORS_Pin, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(SDA_SENSORS_GPIO_Port, SDA_SENSORS_Pin, GPIO_PIN_SET);
	HAL_Delay(1);

	if(HAL_I2C_Init(&hi2c1) != HAL_OK
			|| HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK
			|| HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK){
		printf("I2C1 recovery failed while reinitializing peripheral\r\n");
	}

	Sensors_I2C_Unlock();
}

static uint8_t Sensors_I2C_IsReady(uint16_t address){
	HAL_StatusTypeDef status;

	Sensors_I2C_Lock();
	status = HAL_I2C_IsDeviceReady(&hi2c1, address,
			SENSOR_I2C_READY_TRIALS, SENSOR_I2C_READY_TIMEOUT_MS);
	Sensors_I2C_Unlock();

	return (status == HAL_OK) ? 1U : 0U;
}

static uint8_t Sensors_TemperatureIsValid(float temperature){
	return (temperature > -100.0f && temperature < 150.0f) ? 1U : 0U;
}


void Sensors_Init(){
	MAX31865_Init(&hspi3, CS_ETEMP_GPIO_Port, CS_ETEMP_Pin);

	Sensors_I2C_Lock();
	if(INA219_INIT(&hi2c1, &ina, INA219_I2C_ADDRESS_GND_GND ,INA219_R100, 3.2) != 0){
		printf("ERROR INITIALIZATING INA\r\n");
	}
	Sensors_I2C_Unlock();

	if(Sensors_I2C_IsReady(MS5803_01BA_I2C_LOW_ADDR)){
		Sensors_I2C_Lock();
		MS5803_01BA_INIT(&hi2c1, MS5803_01BA_I2C_LOW_ADDR, &ms5803_prom);
		Sensors_I2C_Unlock();
	}else{
		printf("MS5803 not detected at I2C address 0x77\r\n");
	}
}

uint8_t SensorsSelfCheck(){
	uint8_t all_ok = 1U;
	uint8_t tmp_ready = Sensors_I2C_IsReady(TMP102_I2C_ADDRESS_GND);
	uint8_t sht_ready = Sensors_I2C_IsReady(SHT4X_I2C_ADDRESS);
	uint8_t ina_ready = Sensors_I2C_IsReady(INA219_I2C_ADDRESS_GND_GND);
	uint8_t ms5803_ready = Sensors_I2C_IsReady(MS5803_01BA_I2C_LOW_ADDR);
	uint8_t npa730_ready = Sensors_I2C_IsReady(NPA730_I2C_ADDRESS);
	float internalTemp = -999.0f;
	float externalTemp;
	float sht_temp = 0.0f;
	float sht_hum = 0.0f;
	uint8_t sht_status = 1U;

	if(tmp_ready){
		Sensors_I2C_Lock();
		internalTemp = TMP102_Read_Temp(&hi2c1, TMP102_I2C_ADDRESS_GND);
		Sensors_I2C_Unlock();
	}

	externalTemp = MAX31865_Read_Temp(&hspi3, CS_ETEMP_GPIO_Port, CS_ETEMP_Pin);

	if(sht_ready){
		Sensors_I2C_Lock();
		sht_status = SHT4X_Measure_TempHum_High(&hi2c1, &sht_temp, &sht_hum);
		Sensors_I2C_Unlock();
	}

	printf("=====================================\r\n");
	printf("======= SELF CHECK INITIALIZED ======\r\n");
	printf("=====================================\r\n");

	if(tmp_ready && Sensors_TemperatureIsValid(internalTemp)){
		printf("TMP102 Correctly Initialized\r\n");
	}else{
		printf("TMP102 NOT detected or invalid reading\r\n");
		all_ok = 0U;
	}

	if(externalTemp == -318.65){
		printf("MAX31865 NOT detected or invalid reading\r\n");
		all_ok = 0U;
	}else{
		printf("MAX31865 Correctly Initialized\r\n");
	}

	if(sht_ready && sht_status == 0U
			&& Sensors_TemperatureIsValid(sht_temp)){
		printf("SHT45 Correctly Initialized\r\n");
	}else{
		printf("SHT45 NOT detected or invalid reading\r\n");
		all_ok = 0U;
	}

	if(ina_ready){
		printf("INA219 Correctly Initialized\r\n");
	}else{
		printf("INA219 NOT detected at I2C address 0x40\r\n");
		all_ok = 0U;
	}

	if(ms5803_ready){
		printf("MS5803 Correctly Initialized\r\n");
	}else{
		printf("MS5803 NOT detected at I2C address 0x77\r\n");
		all_ok = 0U;
	}

	if(npa730_ready){
		printf("NPA730 Correctly Initialized\r\n");
	}else{
		printf("NPA730 NOT detected at I2C address 0x28\r\n");
		all_ok = 0U;
	}

	return all_ok;
}

uint8_t readSensors(TelemetryPacket *packet){
	uint8_t success = 1;
	uint8_t i2c_recovery_needed = 0U;
	packet->sync = 0xB1E5;
	packet->error_flags = 0;

	// Internal Temperature
	Sensors_I2C_Lock();
	float tmp_temperature = TMP102_Read_Temp(&hi2c1, TMP102_I2C_ADDRESS_GND);
	Sensors_I2C_Unlock();
	if(tmp_temperature == -999.0f){
		/* A failure at the first sensor normally means the previous cycle left
		 * the shared bus busy. Recover it and retry the primary sensor once. */
		Sensors_I2C_RecoverBus();
		Sensors_I2C_Lock();
		tmp_temperature = TMP102_Read_Temp(&hi2c1, TMP102_I2C_ADDRESS_GND);
		Sensors_I2C_Unlock();
	}
	uint8_t tmp_valid = Sensors_TemperatureIsValid(tmp_temperature);
	if(tmp_valid){
		/* TMP102 is the primary internal-temperature source. */
		packet->int_temp = (int16_t)(tmp_temperature * 100.0f);
	}else{
		success = 0;
		packet->error_flags |= TELEMETRY_ERROR_INT_TEMP;
	}

	// External Temperature
	float externalTemp = MAX31865_Read_Temp(&hspi3, CS_ETEMP_GPIO_Port, CS_ETEMP_Pin);
	if(externalTemp != -318.65){
		packet->ext_temp =(int16_t) (externalTemp * 100);
	}
	else{
		success = 0;
		packet->error_flags |= TELEMETRY_ERROR_EXT_TEMP;
	}

	// Humidity

	float sht_temp = 0.0f;
	float sht_hum = 0.0f;

	Sensors_I2C_Lock();
	uint8_t sht_status = SHT4X_Measure_TempHum_High(&hi2c1, &sht_temp, &sht_hum);
	Sensors_I2C_Unlock();
	if (sht_status == 0U && Sensors_TemperatureIsValid(sht_temp)) {
	    packet->ext_hum = (uint16_t)(sht_hum * 100.0f);

	    /* Use the SHT45 temperature only if the primary TMP102 failed. */
	    if(!tmp_valid){
		packet->int_temp = (int16_t)(sht_temp * 100.0f);
	    }
	} else {
	    success = 0;
	    packet->error_flags |= TELEMETRY_ERROR_EXT_HUM;
	    i2c_recovery_needed = 1U;
	}

	// Current Consumption (mA)
	Sensors_I2C_Lock();
	uint8_t ina_ok = INA219_READ_CURRENT(&hi2c1, &ina);
	Sensors_I2C_Unlock();
	if(ina_ok){
		packet->halow_curr = ina.current;
	}else{
		success = 0;
		packet->error_flags |= TELEMETRY_ERROR_HALOW_CURRENT;
		i2c_recovery_needed = 1U;
	}

	// Pressure
	int32_t raw_temp, raw_press;
	Sensors_I2C_Lock();
	raw_press = MS5803_01BA_READ_RAW(&hi2c1, MS5803_01BA_I2C_LOW_ADDR, MS5803_01BA_CONVERT_PRESSURE_OSR4096);
	raw_temp = MS5803_01BA_READ_RAW(&hi2c1, MS5803_01BA_I2C_LOW_ADDR, MS5803_01BA_CONVERT_TEMP_OSR4096);
	Sensors_I2C_Unlock();
	if(raw_press == 0 || raw_temp == 0){
		i2c_recovery_needed = 1U;
	}


	MS5803_01BA_CALIBRATE_DATA(raw_press, raw_temp, &ms5803_prom, &ms5803);
	packet->env_press = ms5803.pressure;

	if(i2c_recovery_needed){
		Sensors_I2C_RecoverBus();
	}


	return success;
}
