/*
 * telemetry.h
 *
 *  Created on: 17 mar 2026
 *      Author: Mariio
 */

#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

#define TELEMETRY_ERROR_INT_TEMP      (1U << 0)
#define TELEMETRY_ERROR_EXT_TEMP      (1U << 1)
#define TELEMETRY_ERROR_EXT_HUM       (1U << 2)
#define TELEMETRY_ERROR_HALOW_CURRENT (1U << 4)
#define TELEMETRY_ERROR_PRESSURE      (1U << 5)

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
	uint16_t system_state; // SystemState_t encoded as uint16_t (keeps packet layout)

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

uint16_t Telemetry_CRC16_Modbus(const uint8_t *data, uint16_t length);
void Telemetry_Finalize(TelemetryPacket *packet);
void Telemetry_UpdateLatest(const TelemetryPacket *packet);
uint8_t Telemetry_GetLatest(TelemetryPacket *packet);
void printTelemetry(const TelemetryPacket *packet);



#endif /* INC_TELEMETRY_H_ */
