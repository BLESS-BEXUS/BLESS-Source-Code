/*
 * txhalow.c
 *
 *  Created on: 23 jun 2026
 *      Author: Mario
 */

#include "main.h"
#include "txhalow.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

extern uint8_t rx_byte;
extern TelemetryPacket rf_data_cache;
extern uint8_t rx_buffer[512];

_Static_assert(sizeof(EthernetHeader) == 14U, "Unexpected Ethernet header size");
_Static_assert(sizeof(TelemetryPacket) == 53U, "Unexpected telemetry packet size");

static uint16_t Telemetry_CRC16_Modbus(const uint8_t *data, uint16_t length){
	uint16_t crc = 0xFFFFU;

	for(uint16_t i = 0; i < length; i++){
		crc ^= data[i];
		for(uint8_t bit = 0; bit < 8U; bit++){
			crc = (crc & 1U) ? (crc >> 1) ^ 0xA001U : crc >> 1;
		}
	}

	return crc;
}



void uart_flush(UART_HandleTypeDef *huart) {
  uint8_t dummy;
  while (HAL_UART_Receive(huart, &dummy, 1, 5) == HAL_OK);
}

uint8_t wait_for_ok(uint32_t timeout_ms) {
  uint32_t start = HAL_GetTick();
  uint32_t elapsed, remaining, flags;

  while ((elapsed = HAL_GetTick() - start) < timeout_ms) {
    remaining = timeout_ms - elapsed;
    flags = osThreadFlagsWait(0x01, osFlagsWaitAny, remaining);

    if ((flags & osFlagsError) != 0) {
      break; // se acabó el tiempo sin más líneas
    }

    // Ha llegado una línea completa vía interrupción
    if (strstr((char*)rx_buffer, "OK") != NULL) {
      return 1;
    } else {
      // No es el ACK que esperamos: es una línea de stats/RF, no la tires
      HaLow_Extract_Data((char*)rx_buffer, &rf_data_cache);
    }
  }
  return 0;
}

uint8_t Tx_HaLow_Packet(UART_HandleTypeDef *huart, TelemetryPacket rx_packet){

	// Telemetry packets
	TelemetryPacket tx_packet = rx_packet;
	uint8_t tx_frame[sizeof(EthernetHeader) + sizeof(TelemetryPacket)];

	// Ethernet header config
	EthernetHeader eth_hdr = {0};
	uint8_t mac_dest[] = {0x82, 0x59, 0x13, 0x89, 0x90, 0xc0};
	memcpy(eth_hdr.dest_mac, mac_dest, 6);
	eth_hdr.eth_type = 0x0000;

	uart_flush(huart);
	tx_packet.checksum = Telemetry_CRC16_Modbus(
		(const uint8_t *)&tx_packet,
		(uint16_t)(sizeof(TelemetryPacket) - sizeof(tx_packet.checksum))
	);

    // Concat the ethernet header to the packet
    memcpy(tx_frame, &eth_hdr, sizeof(EthernetHeader));
    memcpy(tx_frame + sizeof(EthernetHeader), &tx_packet, sizeof(TelemetryPacket));

    const char *cmd = TX_PACKET_COMMAND;
    HAL_UART_Transmit(huart, (uint8_t*)cmd, strlen(cmd), 100);

    if (wait_for_ok(1000)) {
	      HAL_UART_Transmit(huart, tx_frame, sizeof(tx_frame), 200);
   	      printf("Packet sent.\r\n");
    } else {
   	      printf("AT: OK wasn't received\n");
    }
    return 0;
}

void HaLow_Send_Config_Command(UART_HandleTypeDef *huart, const char *cmd, const char *cmd_value){
	uint8_t ok_received = 0;
	char cmd_buffer[200];

	do{
		snprintf(cmd_buffer, sizeof(cmd_buffer), "AT+%s=%s\r\n", cmd, cmd_value);
		HAL_UART_Transmit(huart, (uint8_t*)cmd_buffer, strlen(cmd_buffer), 100);

	    if (wait_for_ok(1000)) {
	   	      printf("%s Established\r\n", cmd);
	   	      HAL_GPIO_WritePin(GPIO_RED_LED_GPIO_Port, GPIO_RED_LED_Pin, GPIO_PIN_RESET);
	   	      HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_SET);
	   	      ok_received = 1;
	    } else {
	   	      printf("AT: OK wasn't received\r\n");
	   	      HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);
	   	      HAL_GPIO_WritePin(GPIO_RED_LED_GPIO_Port, GPIO_RED_LED_Pin, GPIO_PIN_SET);
	   	      ok_received = 0;
	   	      osDelay(500);
	    }
	} while(!ok_received);

}

uint8_t Config_HaLow(UART_HandleTypeDef *huart){

	printf("WiFi HaLow Module Configuration\r\n");
	osDelay(500);

	HaLow_Send_Config_Command(huart, "MODE", HALOW_MODE);
	HaLow_Send_Config_Command(huart, "SSID", HALOW_SSID);
	HaLow_Send_Config_Command(huart, "PSK", HALOW_PSK);
	HaLow_Send_Config_Command(huart, "BSS_BW", HALOW_BSS_BW);
	HaLow_Send_Config_Command(huart, "FREQ_RANGE", HALOW_FREQ_RANGE);
	HaLow_Send_Config_Command(huart, "ACKTMO", HALOW_ACKTMO);
	HaLow_Send_Config_Command(huart, "TXPOWER", HALOW_TX_POWER);
	HaLow_Send_Config_Command(huart, "TX_MCS", HALOW_TX_MCS);

	return 1;

}


void HaLow_Extract_Data(const char *rx_line, TelemetryPacket *tx_packet){
	char *ptr;
	int int_value;

	// Extract RSSI
	ptr = strstr(rx_line, "rssi=");
	if(ptr != NULL){
		sscanf(ptr, "rssi=%d", &int_value);
		tx_packet->rf_rssi = int_value;
	}

	// Extract SNR
	ptr = strstr(rx_line, "snr=");
	if(ptr != NULL){
		sscanf(ptr, "snr=%d", &int_value);
		tx_packet->rf_snr = int_value;
	}

	// Calculate noise floor

	tx_packet->rf_noise_floor = tx_packet->rf_rssi - tx_packet->rf_snr;

	// Extract Frequency Deviation
	ptr = strstr(rx_line, "freqDev=");
	if(ptr != NULL){
		sscanf(ptr, "freqDev=%d", &int_value);
		tx_packet->rf_freq_dev = int_value;
	}

	// Extract MCS

	ptr = strstr(rx_line, "mcs=");
	if(ptr != NULL){
		sscanf(ptr, "mcs=%d", &int_value);
		tx_packet->rf_tx_mcs = int_value;
	}

	// Extract Chip Temperature
	ptr = strstr(rx_line, "chip-temperature:");
	if(ptr != NULL){
		sscanf(ptr, "chip-temperature:%d", &int_value);
		tx_packet->temp_chip = int_value;
	}

}
