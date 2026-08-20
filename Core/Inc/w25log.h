/*
 * w25log.h
 *
 *  Created on: 30 jun 2026
 *      Author: Mario
 */

#ifndef INC_W25LOG_H_
#define INC_W25LOG_H_


#include "main.h"
#include "w25qxx.h"

#define W25LOG_PACKET_SIZE   53
#define W25LOG_SLOT_SIZE     64                           // 53 + 4 magic + 7 padding
#define W25LOG_SECTOR_SIZE   4096
#define W25LOG_SLOTS_PER_SECTOR (W25LOG_SECTOR_SIZE / W25LOG_SLOT_SIZE)   // 64
#define W25LOG_FLASH_SIZE    (8UL * 1024UL * 1024UL)      // W25Q64 = 8MB
#define W25LOG_TOTAL_SECTORS (W25LOG_FLASH_SIZE / W25LOG_SECTOR_SIZE)     // 2048
#define W25LOG_TOTAL_SLOTS   (W25LOG_FLASH_SIZE / W25LOG_SLOT_SIZE)       // 131072

// MAGIC DEFINES
#define W25LOG_MAGIC0  0x55
#define W25LOG_MAGIC1  0xAA
#define W25LOG_MAGIC2  0x55
#define W25LOG_MAGIC3  0xAA

typedef struct {
    uint32_t next_write_addr;
    uint32_t packet_count;
    uint32_t boot_id;
    uint8_t reset_cause;
} W25LOG_State_t;

typedef void (*W25LOG_ReadCallback)(uint32_t index, const uint8_t *packet_data, void *ctx);


uint8_t W25LOG_Init(W25LOG_State_t *state);

uint8_t W25LOG_Format(W25LOG_State_t *state);

uint8_t W25LOG_WritePacket(W25LOG_State_t *state, const uint8_t *packet_data);

uint8_t W25LOG_ReadLastPacket(W25LOG_State_t *state, uint8_t *out_buf);

void W25LOG_ReadAll(W25LOG_State_t *state, W25LOG_ReadCallback cb, void *ctx);

void W25LOG_DumpToUART(UART_HandleTypeDef *huart, W25LOG_State_t *state);



#endif /* INC_W25LOG_H_ */
