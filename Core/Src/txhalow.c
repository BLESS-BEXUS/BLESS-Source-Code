/*
 * txhalow.c
 *
 *  Created on: 23 jun 2026
 *      Author: Mario
 */

#include "main.h"
#include "txhalow.h"
extern volatile HaLowMode_t halow_active_mode;
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

extern uint8_t rx_byte;
extern TelemetryPacket rf_data_cache;
extern char halow_rx_lines[HALOW_RX_LINE_DEPTH][HALOW_RX_LINE_SIZE];
extern volatile uint8_t halow_rx_line_head;
extern volatile uint8_t halow_rx_line_tail;
extern volatile uint8_t halow_rx_line_count;
extern volatile uint16_t rx_index;

_Static_assert(sizeof(EthernetHeader) == 14U, "Unexpected Ethernet header size");
_Static_assert(sizeof(TelemetryPacket) == 53U, "Unexpected telemetry packet size");



void uart_flush(UART_HandleTypeDef *huart) {
  (void)huart;
  taskENTER_CRITICAL();
  halow_rx_line_head = 0U;
  halow_rx_line_tail = 0U;
  halow_rx_line_count = 0U;
  rx_index = 0U;
  taskEXIT_CRITICAL();
}

uint8_t HaLow_PopRxLine(char *line, uint16_t capacity) {
  if (line == NULL || capacity == 0U) {
    return 0U;
  }

  taskENTER_CRITICAL();
  if (halow_rx_line_count == 0U) {
    taskEXIT_CRITICAL();
    return 0U;
  }

  strncpy(line, halow_rx_lines[halow_rx_line_tail], capacity - 1U);
  line[capacity - 1U] = '\0';
  halow_rx_line_tail = (uint8_t)((halow_rx_line_tail + 1U)
      % HALOW_RX_LINE_DEPTH);
  halow_rx_line_count--;
  taskEXIT_CRITICAL();
  return 1U;
}

uint8_t wait_for_ok(uint32_t timeout_ms) {
  uint32_t start = HAL_GetTick();
  uint32_t elapsed, remaining, flags;
  char line[HALOW_RX_LINE_SIZE];

  while ((elapsed = HAL_GetTick() - start) < timeout_ms) {
    remaining = timeout_ms - elapsed;
    flags = osThreadFlagsWait(0x01, osFlagsWaitAny, remaining);

    if ((flags & osFlagsError) != 0) {
      break; // se acabó el tiempo sin más líneas
    }

    while (HaLow_PopRxLine(line, sizeof(line))) {
      if (strstr(line, "OK") != NULL) {
        return 1U;
      }
      if (strstr(line, "ERROR") != NULL) {
        return 0U;
      }
      HaLow_Extract_Data(line, &rf_data_cache);
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

	    if (halow_active_mode == HALOW_MODE_GROUP) {
        const uint8_t dest[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        memcpy(eth_hdr.dest_mac, dest, sizeof(dest));
    } else {
        const uint8_t dest[6] = {0x82,0x59,0x13,0x89,0x90,0xC0};
        memcpy(eth_hdr.dest_mac, dest, sizeof(dest));
    }
	eth_hdr.eth_type = 0x0000;

	uart_flush(huart);
	Telemetry_Finalize(&tx_packet);

    // Concat the ethernet header to the packet
    memcpy(tx_frame, &eth_hdr, sizeof(EthernetHeader));
    memcpy(tx_frame + sizeof(EthernetHeader), &tx_packet, sizeof(TelemetryPacket));

    const char *cmd = TX_PACKET_COMMAND;
    if (HAL_UART_Transmit(huart, (uint8_t*)cmd, strlen(cmd), 100) != HAL_OK) {
        return 0U;
    }

    if (wait_for_ok(1000)) {
        if (HAL_UART_Transmit(huart, tx_frame, sizeof(tx_frame), 200) != HAL_OK) {
            return 0U;
        }
        printf("Packet sent.\r\n");
        return 1U;
    } else {
   	      printf("AT: OK wasn't received\n");
    }
    return 0U;
}

uint8_t HaLow_Send_Config_Command(UART_HandleTypeDef *huart, const char *cmd, const char *cmd_value){
	char cmd_buffer[200];

	for(uint8_t attempt = 0U; attempt < HALOW_CONFIG_MAX_ATTEMPTS; ++attempt){
		snprintf(cmd_buffer, sizeof(cmd_buffer), "AT+%s=%s\r\n", cmd, cmd_value);
		uart_flush(huart);
		if(HAL_UART_Transmit(huart, (uint8_t*)cmd_buffer,
				strlen(cmd_buffer), 100) != HAL_OK){
			osDelay(100);
			continue;
		}

	    if (wait_for_ok(1000)) {
	        printf("%s Established\r\n", cmd);
	        HAL_GPIO_WritePin(GPIO_RED_LED_GPIO_Port, GPIO_RED_LED_Pin, GPIO_PIN_RESET);
	        HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_SET);
	        return 1U;
	    } else {
	   	      printf("AT: OK wasn't received\r\n");
	   	      HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);
	   	      HAL_GPIO_WritePin(GPIO_RED_LED_GPIO_Port, GPIO_RED_LED_Pin, GPIO_PIN_SET);
	   	      osDelay(500);
	    }
	}

	printf("AT+%s was not accepted after %u attempts\r\n",
	       cmd, HALOW_CONFIG_MAX_ATTEMPTS);
	return 0U;
}

uint8_t Config_HaLow(UART_HandleTypeDef *huart, HaLowMode_t mode)
{
    printf("WiFi HaLow Module Configuration\r\n");
    osDelay(500);
    if (!HaLow_Send_Config_Command(huart, "MODE", mode == HALOW_MODE_GROUP ? "GROUP" : "AP"))
    {
    	return 0U;
    }

if (mode == HALOW_MODE_NORMAL) {

    if (!HaLow_Send_Config_Command(huart, "SSID", HALOW_SSID))
    {
    	return 0U;
    }

    if (!HaLow_Send_Config_Command(huart, "PSK", HALOW_PSK))
    {
    	return 0U;
    }


} 

    if (!HaLow_Send_Config_Command(huart, "BSS_BW", HALOW_BSS_BW))
    {
    	return 0U;
    }


    if (!HaLow_Send_Config_Command(huart, "FREQ_RANGE", HALOW_FREQ_RANGE))
    {
    	return 0U;
    }


if (mode == HALOW_MODE_NORMAL) {

    if (!HaLow_Send_Config_Command(huart, "ACKTMO", HALOW_ACKTMO))
    {
    	return 0U;
    }


} 

    if (!HaLow_Send_Config_Command(huart, "TXPOWER", HALOW_TX_POWER))
    {
    	return 0U;
    }


    if (!HaLow_Send_Config_Command(huart, "TX_MCS", HALOW_TX_MCS))
    {
    	return 0U;
    }

    return 1U;
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

	/* The module reports packet error rate (PER); telemetry carries packet
	 * delivery/success rate (PDR), hence 100 - PER. */
	ptr = strstr(rx_line, "per=");
	if(ptr != NULL && sscanf(ptr, "per=%d", &int_value) == 1){
		if(int_value < 0) int_value = 0;
		if(int_value > 100) int_value = 100;
		tx_packet->rf_success_rate = (uint8_t)(100 - int_value);
	}

	// Extract Chip Temperature
	ptr = strstr(rx_line, "chip-temperature:");
	if(ptr != NULL){
		sscanf(ptr, "chip-temperature:%d", &int_value);
		tx_packet->temp_chip = int_value;
	}

}
