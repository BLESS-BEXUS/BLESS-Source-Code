/*
 * elink_cmd.c
 *
 *  Created on: 25 jun 2026
 *      Author: Mario
 */


#include "main.h"
#include "txhalow.h"
#include "elink_cmd.h"


/** UTILITY FUNCTIONS **/
uint16_t Calculate_Checksum(ELinkCommand *command){
	uint16_t sum = 0;
	uint8_t *ptr = (uint8_t *)command;
	uint16_t length_to_check = sizeof(ELinkCommand) - sizeof(uint16_t);

	for(uint16_t i = 0; i < length_to_check; i++){
		sum += ptr[i];
	}
	return sum;
}

uint8_t RECEIVE_ELINK_CMD(ELinkCommand *command){
	// Check Sync
	if(command->sync1 != 0xE175 || command->sync2 != 0xC245){
		return 1;
	}
	// Checksum
	if(Calculate_Checksum(command) != command->checksum){
		return 2;
	}

	switch(command->cmd_id){
		// System Commands
		case 0x01:
			CMD_SYS_RESET();
			break;
		case 0x02:
			CMD_SYS_START();
			break;
		case 0x03:
			CMD_SYS_STOP();
			break;
		case 0x04:
			CMD_PING();
			break;

		// Survival Commands
		case 0x10:
			CMD_HEAT_MANUAL(command->payload[0]);
			break;
		case 0x11:
			CMD_HEAT_AUTO();
			break;

		// RF Commands
		case 0x20:
			CMD_HALOW_CONNECT();
			break;
		case 0x21:
			CMD_SET_TX_POWER(command->payload[0]);
			break;
		case 0x22:
			CMD_SET_MCS_MODE(command->payload[0]);
			break;
		case 0x23:
			CMD_RF_SILENCE();
			break;

		// Data Management Commands
		case 0x30:
			CMD_SAVE_DATA();
			break;
		case 0x31:
			CMD_SEND_HALOW();
			break;
		case 0x32:
			CMD_SEND_ELINK();
			break;

		default:
			return 3;
	}
	return 0;
}


/**** COMMAND FUNCTIONS ****/

/** SYSTEM COMMANDS **/

void CMD_SYS_RESET(){
	NVIC_SystemReset();
}

void CMD_SYS_START(){
	current_state = STATE_FLIGHT;
}

void CMD_SYS_STOP(){
	current_state = STATE_IDDLE;
}

void CMD_PING(){
	flag_ping = 1;
}

/*** SURVIVAL COMMANDS ***/
uint8_t CMD_HEAT_MANUAL(uint8_t value){
	return 0;
}
void CMD_HEAT_AUTO(){

}

/*** RF COMMANDS ***/
void CMD_HALOW_CONNECT(){

}
void CMD_SET_TX_POWER(uint8_t tx_power){
	new_tx_power = tx_power;
	flag_update_tx_power = 1;
}

void CMD_SET_MCS_MODE(uint8_t mcs_mode){
	new_mcs = mcs_mode;
	flag_update_mcs = 1;
}

void CMD_RF_SILENCE(){

}

/*** DATA MANAGEMENT COMMANDS ***/
void CMD_SAVE_DATA(){
	flag_dump_spi_data = 1;
}
void CMD_SEND_HALOW(){

}
void CMD_SEND_ELINK(){

}
