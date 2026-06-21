/*
 * telemetry.c
 *
 *  Created on: 17 mar 2026
 *      Author: Mariio
 */

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "telemetry.h"

extern UART_HandleTypeDef huart2;

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void printTelemetry(TelemetryPacket *packet){

	printf("----- TELEMETRY PACKET -----\r\n");

	// Header and Timing
	printf("Packet ID: %lu | Time: %lu s\r\n", packet->count, packet->timestamp);

	// Radio Performance
	printf("Radio RSSI: ");
	if(packet->rf_rssi == 0){
		printf("DISCONNECTED | ");
	}else{
		printf("%d dBm | ", packet->rf_rssi);
	}
	printf("TX SNR: %u | TX Noise Floor: %u\r\n", packet->rf_snr, packet->rf_noise_floor);

	// Power Data
	printf("Voltage: %.3f V | Current: %u mA\r\n", packet->halow_volt / 1000.0, packet->halow_curr);

	// Sensor Data
	printf("Temperature - EXT: %.2f C | INT: %.2f C\r\n", packet->ext_temp / 100.0, packet->int_temp / 100.0);
	printf("Humidity: %.2f%% \r\n", packet->ext_hum / 100.0);

	// System State and Security
	printf("Checksum: 0x%04X\r\n", packet->checksum);

	if(packet->error_flags != 0){
		printf("ERROR FLAGS : 0x%02X\r\n", packet->error_flags);

		if(packet->error_flags & 0x01){
			printf(" [Error: TMP102]\r\n");
		}
		if(packet->error_flags & 0x02){
			printf(" [Error: PT100]\r\n");
		}
		if(packet->error_flags & 0x04){
			printf(" [Error: SD Card]\r\n");
		}
	}

	printf("----------------------------\r\n\n");
}
