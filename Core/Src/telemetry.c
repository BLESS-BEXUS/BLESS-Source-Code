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
	printf("--- RF METRICS ---\r\n");
	printf("Radio RSSI: ");
	if(packet->rf_rssi == 0){
		printf("DISCONNECTED | ");
	}else{
		printf("%d dBm | ", packet->rf_rssi);
	}
	printf("TX SNR: %u | TX Noise Floor: %d\r\n", packet->rf_snr, packet->rf_noise_floor);
	printf("RF Freq Dev: %d | RF MCS: %u\r\n", packet->rf_freq_dev, packet->rf_tx_mcs);
	printf("RF Chip Temp: %d\r\n", packet->temp_chip);

	// GNSS Data
	printf("--- GNSS METRICS ---\r\n");

	int8_t raw_fix_sats = packet->gps_fix_sats;
	uint8_t num_satellites = raw_fix_sats & 0x3F;
	uint8_t fix_quality = (raw_fix_sats >> 6) & 0x03;

	printf("GPS Fix: ");
	switch(fix_quality) {
		case 0:  printf("NO FIX"); break;
		case 1:  printf("3D FIX"); break;
		case 2:  printf("DGPS FIX"); break;
		case 3:  printf("RTK FIX"); break;
		default: printf("UNKNOWN"); break;
	}
	printf(" | Satellites: %u\r\n", num_satellites);
	printf("Latitude: %.7f | Longitude: %.7f\r\n", packet->gps_lat / 10000000.0, packet->gps_lon / 10000000.0);
	printf("Altitude: %u m\r\n", packet->gps_alt);

	// Power Data
	printf("--- POWER ---\r\n");
	printf("Current: %u mA\r\n", packet->halow_curr);

	// Sensor Data
	printf("--- SENSORS ---\r\n");
	printf("Temperature - EXT: %.2f C | INT: %.2f C\r\n", packet->ext_temp / 100.0, packet->int_temp / 100.0);
	printf("Humidity: %.2f%% \r\n", packet->ext_hum / 100.0);
	printf("Radiation: %d cps\r\n", packet->rad_cps);

	// System State and Security
	printf("--- SYSTEM STATUS ---\r\n");
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
