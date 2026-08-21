/*
 * elink_cmd.c
 *
 *  Created on: 25 jun 2026
 *      Author: Mario
 */


#include "main.h"
#include "txhalow.h"
#include "elink_cmd.h"
#include "mission_time.h"
#include "cmsis_os.h"

extern osMessageQueueId_t txElinkMailBoxHandle;


/** UTILITY FUNCTIONS **/
uint16_t Calculate_Checksum(const ELinkCommand *command){
	uint16_t sum = 0;
	const uint8_t *ptr = (const uint8_t *)command;
	uint16_t length_to_check = sizeof(ELinkCommand) - sizeof(uint16_t);

	for(uint16_t i = 0; i < length_to_check; i++){
		sum += ptr[i];
	}
	return sum;
}

ELinkCommandResult RECEIVE_ELINK_CMD(const ELinkCommand *command){
	// Check Sync
	if(command->sync1 != ELINK_SYNC1 || command->sync2 != ELINK_SYNC2){
		return ELINK_CMD_ERR_SYNC;
	}
	// Checksum
	if(Calculate_Checksum(command) != command->checksum){
		return ELINK_CMD_ERR_CHECKSUM;
	}
	if(command->payload_size > MAX_PAYLOAD_SIZE){
		return ELINK_CMD_ERR_PAYLOAD;
	}

	switch(command->cmd_id){

	    // System Commands
	    case 0x01:
	        if(command->payload_size != ELINK_SYS_RESET_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_SYS_RESET();
	        break;

	    case 0x02:
	        if(command->payload_size != ELINK_SYS_START_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_SYS_START();
	        break;

	    case 0x03:
	        if(command->payload_size != ELINK_SYS_STOP_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_SYS_STOP();
	        break;

	    case ELINK_CMD_PING:
	        if(command->payload_size != ELINK_PING_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        break;

	    case ELINK_CMD_SYNC_TIME:
	        if(command->payload_size != ELINK_SYNC_TIME_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_SYNC_TIME();
	        break;

	    // Survival Commands
	    case 0x10:
	        if(command->payload_size != ELINK_HEAT_MANUAL_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }

	        if(CMD_HEAT_MANUAL(command->payload[0]) == 0U){
	            return ELINK_CMD_ERR_RANGE;
	        }
	        break;

	    case 0x11:
	        if(command->payload_size != ELINK_HEAT_AUTO_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_HEAT_AUTO();
	        break;

	    // RF Commands
	    case 0x20:
	        if(command->payload_size != ELINK_HALOW_CONNECT_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_HALOW_CONNECT();
	        break;

	    case 0x21:
	        if(command->payload_size != ELINK_SET_TX_POWER_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }

	        if(command->payload[0] < HALOW_TX_POWER_MIN_DBM ||
	           command->payload[0] > HALOW_TX_POWER_MAX_DBM){
	            return ELINK_CMD_ERR_RANGE;
	        }

	        CMD_SET_TX_POWER(command->payload[0]);
	        break;

	    case 0x22:
	        if(command->payload_size != ELINK_SET_TX_MCS_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }

	        if(command->payload[0] < HALOW_TX_MCS_MIN ||
	           command->payload[0] > HALOW_TX_MCS_MAX){
	            return ELINK_CMD_ERR_RANGE;
	        }

	        CMD_SET_MCS_MODE(command->payload[0]);
	        break;

	    case 0x23:
	        if(command->payload_size != ELINK_RF_SILENCE_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_RF_SILENCE();
	        break;

	    // Data Management Commands
	    case 0x30:
	        if(command->payload_size != ELINK_SAVE_DATA_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_SAVE_DATA();
	        break;

	    case 0x31:
	        if(command->payload_size != ELINK_SEND_HALOW_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_SEND_HALOW();
	        break;

	    case 0x32:
	        if(command->payload_size != ELINK_SEND_ELINK_PAYLOAD_SIZE){
	            return ELINK_CMD_ERR_PAYLOAD;
	        }
	        CMD_SEND_ELINK();
	        break;

	    default:
	        return ELINK_CMD_ERR_UNKNOWN_ID;
	}

	return ELINK_CMD_OK;
}

void Build_Ping_Response(const ELinkCommand *request, ELinkCommand *response){
	response->sync1 = ELINK_SYNC1;
	response->sync2 = ELINK_SYNC2;
	response->cmd_id = ELINK_RSP_PONG;
	response->payload_size = ELINK_PING_PAYLOAD_SIZE;
	response->payload[0] = request->payload[0];
	response->payload[1] = request->payload[1];
	response->checksum = Calculate_Checksum(response);
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

void CMD_SYNC_TIME(void){
	MissionTime_Sync();
}

/*** SURVIVAL COMMANDS ***/
uint8_t CMD_HEAT_MANUAL(uint8_t value){
	if(value > 100U){
		return 0U;
	}

	heater_manual_power_pct = value;
	heater_manual_enabled = 1U;
	return 1U;
}
void CMD_HEAT_AUTO(){
	heater_manual_enabled = 0U;
}

/*** RF COMMANDS ***/
void CMD_HALOW_CONNECT(){
	halow_rf_silenced = 0U;
	flag_halow_connect = 1U;
	flag_force_halow_tx = 1U;
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
	/* Software TX inhibit: the published T-HaLow AT interface has no
	 * documented RF-off command. In AP mode the module may still emit its
	 * own management/beacon frames even though BLESS sends no payloads. */
	halow_rf_silenced = 1U;
	flag_force_halow_tx = 0U;
}

/*** DATA MANAGEMENT COMMANDS ***/
void CMD_SAVE_DATA(){
	flag_save_data = 1U;
}
void CMD_SEND_HALOW(){
	if(halow_rf_silenced == 0U){
		flag_force_halow_tx = 1U;
	}
}
void CMD_SEND_ELINK(){
	TelemetryPacket packet;
	TelemetryPacket stale_packet;

	if(Telemetry_GetLatest(&packet) == 0U){
		return;
	}

	if(osMessageQueuePut(txElinkMailBoxHandle, &packet, 0U, 0U) != osOK){
		(void)osMessageQueueGet(txElinkMailBoxHandle, &stale_packet, NULL, 0U);
		(void)osMessageQueuePut(txElinkMailBoxHandle, &packet, 0U, 0U);
	}
}
