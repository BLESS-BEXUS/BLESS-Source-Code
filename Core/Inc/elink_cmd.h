/*
 * elink_cmd.h
 *
 *  Created on: 23 jun 2026
 *      Author: Mario
 */

#ifndef INC_ELINK_CMD_H_
#define INC_ELINK_CMD_H_

/******* E-LINK COMMAND PACKET STRUCTURE *******/

#include "main.h"

#define MAX_PAYLOAD_SIZE 2

typedef struct{
	// HEADER
	uint16_t sync1;
	uint16_t sync2;

	// COMMAND DATA
	uint8_t cmd_id;
	uint8_t payload_size;
	uint8_t payload[MAX_PAYLOAD_SIZE];

	// FOOTER
	uint16_t checksum;
} __attribute__((packed)) ELinkCommand;

extern volatile SystemState_t current_state;

extern volatile uint8_t new_tx_power;
extern volatile uint8_t flag_update_tx_power;

extern volatile uint8_t new_mcs;
extern volatile uint8_t flag_update_mcs;

extern volatile uint8_t flag_dump_spi_data;

extern volatile uint8_t flag_ping;

/** UTILITY FUNCTIONS **/

void Config_Ethernet();
uint16_t Calculate_Checksum(ELinkCommand *command);
uint8_t RECEIVE_ELINK_CMD(ELinkCommand *command);

/**** COMMAND FUNCTIONS ****/

// SYSTEM COMMANDS
void CMD_SYS_RESET();
void CMD_SYS_START();
void CMD_SYS_STOP();
void CMD_PING();

// SURVIVAL COMMANDS
uint8_t CMD_HEAT_MANUAL(uint8_t value);
void CMD_HEAT_AUTO();

// RADIO FREQUENCY COMMANDS
void CMD_HALOW_CONNECT();
void CMD_SET_TX_POWER(uint8_t tx_power);
void CMD_SET_MCS_MODE(uint8_t mcs_mode);
void CMD_RF_SILENCE();

// DATA COMMANDS
void CMD_SAVE_DATA();
void CMD_SEND_HALOW();
void CMD_SEND_ELINK();

#endif /* INC_ELINK_CMD_H_ */
