/*
 * telemetry.h
 *
 *  Created on: 17 mar 2026
 *      Author: Mariio
 */

#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

typedef struct TelemetryPacket{
	// HEADER
	uint16_t sync;
	uint32_t timestamp;
	uint32_t count;

	// GNSS
	int32_t gps_lat;
	int32_t gps_lon;
	uint16_t gps_alt;
	int8_t gps_fix_sats;

	// RF PERFORMANCE
	int16_t rf_rssi;
	int8_t rf_snr;
	int16_t rf_freq_dev;
	uint8_t rf_success_rate;
	int16_t rf_noise_floor;
	uint8_t rf_tx_mcs;
	uint16_t rf_volt; // RF Log Detector

	// HARDWARE
	uint16_t halow_curr;
	int16_t temp_chip;
	uint16_t heater_power;

	// ENVIROMENT
	uint32_t env_press;
	int16_t ext_temp;
	int16_t int_temp;
	uint16_t ext_hum;
	uint16_t rad_cps;

	// FOOTER
	uint8_t error_flags;
	uint16_t checksum;
} __attribute__((packed)) TelemetryPacket;

void printTelemetry();



#endif /* INC_TELEMETRY_H_ */
