/*
 * w25qxx.h
 *
 *  Created on: 30 jun 2026
 *      Author: Mario
 */

#ifndef INC_W25QXX_H_
#define INC_W25QXX_H_

#include "main.h"

/* --- W25QXX Commands --- */
#define W25X_WriteEnable      0x06
#define W25X_WriteDisable     0x04
#define W25X_ReadStatusReg1   0x05
#define W25X_ReadData         0x03
#define W25X_PageProgram      0x02
#define W25X_SectorErase      0x20
#define W25X_ChipErase        0xC7
#define W25X_JedecID          0x9F
#define W25QXX_READY_TIMEOUT_MS 1000U

/* --- W25Q64 at SPI1 (PA5=SCK, PA6=MISO, PA7=MOSI, PB6=CS) --- */
#define W25QXX_SPI_HANDLE     hspi1
#define W25QXX_CS_PORT        GPIOB
#define W25QXX_CS_PIN         GPIO_PIN_6

typedef struct {
    uint8_t  ManufacturerID;
    uint8_t  MemoryType;
    uint8_t  Capacity;
} W25QXX_ID_t;

uint8_t W25QXX_Init(W25QXX_ID_t *id);
void    W25QXX_ReadID(W25QXX_ID_t *id);
uint8_t W25QXX_ReadData(uint32_t addr, uint8_t *buf, uint32_t len);
uint8_t W25QXX_PageProgram(uint32_t addr, uint8_t *buf, uint16_t len);
uint8_t W25QXX_SectorErase(uint32_t addr);
uint8_t W25QXX_ReadStatus(void);
uint8_t W25QXX_WaitBusy(uint32_t timeout_ms);


#endif /* INC_W25QXX_H_ */
