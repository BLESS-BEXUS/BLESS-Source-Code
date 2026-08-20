/*
 * w25qxx.c
 *
 *  Created on: 30 jun 2026
 *      Author: Mario
 */


#include "w25qxx.h"
#include "cmsis_os.h"

#include <string.h>

extern SPI_HandleTypeDef W25QXX_SPI_HANDLE;

static inline void CS_LOW(void)  { HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET); }
static inline void CS_HIGH(void) { HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET); }

static uint8_t transaction_ok = 1U;

static uint8_t SPI_TxRx(uint8_t data) {
    uint8_t rx = 0xFFU;
    if (HAL_SPI_TransmitReceive(&W25QXX_SPI_HANDLE, &data, &rx, 1, 100) != HAL_OK) {
        transaction_ok = 0U;
    }
    return rx;
}

/* --- Read JEDEC ID (0x9F) --- */
void W25QXX_ReadID(W25QXX_ID_t *id) {
    CS_LOW();
    SPI_TxRx(W25X_JedecID);
    id->ManufacturerID = SPI_TxRx(0xFF);
    id->MemoryType     = SPI_TxRx(0xFF);
    id->Capacity        = SPI_TxRx(0xFF);
    CS_HIGH();
}

uint8_t W25QXX_ReadStatus(void) {
    uint8_t status;
    transaction_ok = 1U;
    CS_LOW();
    SPI_TxRx(W25X_ReadStatusReg1);
    status = SPI_TxRx(0xFF);
    CS_HIGH();
    return status;
}

uint8_t W25QXX_WaitBusy(uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();

    while (W25QXX_ReadStatus() & 0x01U) {
        if (transaction_ok == 0U || (HAL_GetTick() - start) >= timeout_ms) {
            return 0U;
        }
        osDelay(1);
    }
    return transaction_ok;
}

static void W25QXX_WriteEnable(void) {
    CS_LOW();
    SPI_TxRx(W25X_WriteEnable);
    CS_HIGH();
}

uint8_t W25QXX_ReadData(uint32_t addr, uint8_t *buf, uint32_t len) {
    transaction_ok = 1U;
    CS_LOW();
    SPI_TxRx(W25X_ReadData);
    SPI_TxRx((addr >> 16) & 0xFF);
    SPI_TxRx((addr >> 8) & 0xFF);
    SPI_TxRx(addr & 0xFF);
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = SPI_TxRx(0xFF);
    }
    CS_HIGH();
    return transaction_ok;
}

uint8_t W25QXX_PageProgram(uint32_t addr, uint8_t *buf, uint16_t len) {
    transaction_ok = 1U;
    W25QXX_WriteEnable();
    CS_LOW();
    SPI_TxRx(W25X_PageProgram);
    SPI_TxRx((addr >> 16) & 0xFF);
    SPI_TxRx((addr >> 8) & 0xFF);
    SPI_TxRx(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        SPI_TxRx(buf[i]);
    }
    CS_HIGH();
    if (transaction_ok == 0U) {
        return 0U;
    }
    return W25QXX_WaitBusy(W25QXX_READY_TIMEOUT_MS);
}

uint8_t W25QXX_SectorErase(uint32_t addr) {
    transaction_ok = 1U;
    W25QXX_WriteEnable();
    CS_LOW();
    SPI_TxRx(W25X_SectorErase);
    SPI_TxRx((addr >> 16) & 0xFF);
    SPI_TxRx((addr >> 8) & 0xFF);
    SPI_TxRx(addr & 0xFF);
    CS_HIGH();
    if (transaction_ok == 0U) {
        return 0U;
    }
    return W25QXX_WaitBusy(W25QXX_READY_TIMEOUT_MS);
}

uint8_t W25QXX_Init(W25QXX_ID_t *id) {
    transaction_ok = 1U;
    CS_HIGH();
    osDelay(10);
    W25QXX_ReadID(id);

    if (transaction_ok == 0U || id->ManufacturerID == 0x00 || id->ManufacturerID == 0xFF) {
        return 0;
    }
    return 1;
}
