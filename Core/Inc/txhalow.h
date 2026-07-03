/*
 * txhalow.h
 *
 *  Created on: 23 jun 2026
 *      Author: Mario
 */

#ifndef INC_TXHALOW_H_
#define INC_TXHALOW_H_

#include "telemetry.h"

/************* COMMAND DEFINES *************/
#define TX_PACKET_COMMAND "AT+TXDATA=67\r\n" // 53 bytes (packet) + 14 bytes (ethernet header)

/************* RF CONFIG DEFINES *************/
#define HALOW_MODE "AP"
#define HALOW_SSID "bless"
#define HALOW_PSK "78a0c6ee134cde7516af41db0fb52cdb911a2fe09420eb0c170e7a88a57f3714"
#define HALOW_BSS_BW "1"
#define HALOW_FREQ_RANGE "8668,8678"
#define HALOW_ACKTMO "2990" // 10 * (Distance in km - 1) assuming 300km link
#define HALOW_TX_POWER "9" // TX power fixed to 9 dbms because LNA will amplify 5dbs
#define HALOW_TX_MCS "0"

#define DUTY_CYCLE_MS 3000

// Struct for the ethernet header
typedef struct {
    uint8_t dest_mac[6]; // MAC of the RX module
    uint8_t src_mac[6];  // Origin Module
    uint16_t eth_type;   // Protocol type
} __attribute__((packed)) EthernetHeader;

/** VOLATILE VARIABLES **/



/** Function Definitions **/

void uart_flush(UART_HandleTypeDef *huart);
uint8_t wait_for_ok(uint32_t timeout_ms);
uint8_t Tx_HaLow_Packet(UART_HandleTypeDef *huart, TelemetryPacket packet);
void HaLow_Send_Config_Command(UART_HandleTypeDef *huart, const char *cmd, const char *cmd_value);
uint8_t Config_HaLow();
void HaLow_Extract_Data(const char *rx_line, TelemetryPacket *tx_packet);

#endif /* INC_TXHALOW_H_ */
