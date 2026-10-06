/*
 * wizchip_port.c
 *
 *  Created on: 3 jul 2026
 *      Author: Mario
 */



#include "wizchip_port.h"
#include "wizchip_conf.h"
#include "cmsis_os.h"
#include "socket.h"
#include <stdio.h>
#include <string.h>

extern SPI_HandleTypeDef hspi2;

#define W5500_MAX_ATOMIC_DATA 2048U

static uint8_t w5500_frame_tx[3U + W5500_MAX_ATOMIC_DATA]
    __attribute__((aligned(4)));
static uint8_t w5500_frame_rx[3U + W5500_MAX_ATOMIC_DATA]
    __attribute__((aligned(4)));

static void W5500_EnterCritical(void) {
    vTaskSuspendAll();
}

static void W5500_ExitCritical(void) {
    (void)xTaskResumeAll();
}

static void W5500_BuildFrameHeader(uint32_t addr_sel) {
    w5500_frame_tx[0] = (uint8_t)((addr_sel & 0x00FF0000U) >> 16);
    w5500_frame_tx[1] = (uint8_t)((addr_sel & 0x0000FF00U) >> 8);
    w5500_frame_tx[2] = (uint8_t)(addr_sel & 0x000000FFU);
}

int8_t W5500_SPI_ReadFrame(uint32_t addr_sel, uint8_t *data, uint16_t length) {
    if ((data == NULL) || (length == 0U) || (length > W5500_MAX_ATOMIC_DATA)) {
        return -1;
    }

    W5500_BuildFrameHeader(addr_sel);
    memset(&w5500_frame_tx[3], 0xFF, length);

    W5500_Select();
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        &hspi2, w5500_frame_tx, w5500_frame_rx,
        (uint16_t)(3U + length), 100U
    );
    W5500_Deselect();

    if (status != HAL_OK) {
        return -1;
    }

    memcpy(data, &w5500_frame_rx[3], length);
    return 0;
}

int8_t W5500_SPI_WriteFrame(uint32_t addr_sel, const uint8_t *data,
                            uint16_t length) {
    if ((data == NULL) || (length == 0U) || (length > W5500_MAX_ATOMIC_DATA)) {
        return -1;
    }

    W5500_BuildFrameHeader(addr_sel);
    memcpy(&w5500_frame_tx[3], data, length);

    W5500_Select();
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        &hspi2, w5500_frame_tx, w5500_frame_rx,
        (uint16_t)(3U + length), 100U
    );
    W5500_Deselect();

    return (status == HAL_OK) ? 0 : -1;
}

void W5500_Select(void) {
    HAL_GPIO_WritePin(CS_ETH_GPIO_Port, CS_ETH_Pin, GPIO_PIN_RESET);
}

void W5500_Deselect(void) {
    HAL_GPIO_WritePin(CS_ETH_GPIO_Port, CS_ETH_Pin, GPIO_PIN_SET);
}

/**
 * @brief Read one byte while generating SPI clock.
 */
uint8_t W5500_ReadByte(void) {
    uint8_t dummy = 0xFFU;
    uint8_t rb = 0U;
    (void)HAL_SPI_TransmitReceive(&hspi2, &dummy, &rb, 1U, 100U);
    return rb;
}

/**
 * @brief Send one byte
 */
void W5500_WriteByte(uint8_t wb) {
    uint8_t discard = 0U;
    (void)HAL_SPI_TransmitReceive(&hspi2, &wb, &discard, 1U, 100U);
}

/**
 * @brief Init and config the W5500 module
 */
int8_t W5500_Init(void) {
    printf("Configuring W5500...\r\n");

    W5500_Deselect();
    HAL_GPIO_WritePin(RST_ETH_GPIO_Port, RST_ETH_Pin, GPIO_PIN_RESET);
    osDelay(100);
    HAL_GPIO_WritePin(RST_ETH_GPIO_Port, RST_ETH_Pin, GPIO_PIN_SET);
    osDelay(500);

    reg_wizchip_cs_cbfunc(W5500_Select, W5500_Deselect);
    reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
    reg_wizchip_cris_cbfunc(W5500_EnterCritical, W5500_ExitCritical);

    uint8_t version = getVERSIONR();
    printf("W5500 Version Register: 0x%02X (expected 0x04)\r\n", version);
    if (version != 0x04U) {
        return -1;
    }

    uint8_t tx_mem_size[8] = {2, 2, 2, 2, 2, 2, 2, 2};
    uint8_t rx_mem_size[8] = {2, 2, 2, 2, 2, 2, 2, 2};

    if (wizchip_init(tx_mem_size, rx_mem_size) < 0) {
        printf("Error, Ethernet not responding.\r\n");
        return -1;
    }

    if (getVERSIONR() != 0x04U) {
        printf("W5500 communication lost during initialization.\r\n");
        return -1;
    }

    // Establish Static IP
    wiz_NetInfo netInfo = {
        .mac = {0x00, 0x08, 0xDC, 0xAB, 0xCD, 0xEF}, // MAC
        .ip  = {172, 16, 18, 152},                   // Flight Segment IP
        .sn  = {255, 255, 255, 0},                   // Netmask
        .gw  = {0, 0, 0, 0},                   // Default gateway
        .dhcp = NETINFO_STATIC
    };

    wiz_NetInfo verifyInfo;
    uint8_t network_configured = 0U;

    for (uint8_t attempt = 0U; attempt < 3U; ++attempt) {
        wizchip_setnetinfo(&netInfo);
        memset(&verifyInfo, 0, sizeof(verifyInfo));
        wizchip_getnetinfo(&verifyInfo);

        if ((memcmp(verifyInfo.mac, netInfo.mac, sizeof(netInfo.mac)) == 0)
            && (memcmp(verifyInfo.ip, netInfo.ip, sizeof(netInfo.ip)) == 0)
            && (memcmp(verifyInfo.sn, netInfo.sn, sizeof(netInfo.sn)) == 0)
            && (memcmp(verifyInfo.gw, netInfo.gw, sizeof(netInfo.gw)) == 0)) {
            network_configured = 1U;
            break;
        }
        osDelay(10);
    }

    if (network_configured == 0U) {
        printf("W5500 network register verification failed.\r\n");
        return -1;
    }

    printf("W5500 initialized.\r\n");
    printf(" -> IP: %u.%u.%u.%u\r\n", verifyInfo.ip[0], verifyInfo.ip[1], verifyInfo.ip[2], verifyInfo.ip[3]);
    printf(" -> Gateway: %u.%u.%u.%u\r\n", verifyInfo.gw[0], verifyInfo.gw[1], verifyInfo.gw[2], verifyInfo.gw[3]);

    return 1;
}
