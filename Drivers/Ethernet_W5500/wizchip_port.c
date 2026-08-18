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

extern SPI_HandleTypeDef hspi2;

void W5500_Select(void) {
    HAL_GPIO_WritePin(CS_ETH_GPIO_Port, CS_ETH_Pin, GPIO_PIN_RESET);
}

void W5500_Deselect(void) {
    HAL_GPIO_WritePin(CS_ETH_GPIO_Port, CS_ETH_Pin, GPIO_PIN_SET);
}

/**
 * @brief Read one byte, sending a dummy packet 0x00
 */
uint8_t W5500_ReadByte(void) {
    uint8_t rb = 0;
    HAL_SPI_Receive(&hspi2, &rb, 1, HAL_MAX_DELAY);
    return rb;
}

/**
 * @brief Send one byte
 */
void W5500_WriteByte(uint8_t wb) {
    HAL_SPI_Transmit(&hspi2, &wb, 1, HAL_MAX_DELAY);
}

/**
 * @brief Init and config the W5500 module
 */
int8_t W5500_Init(void) {
    printf("Configuring W5500...\r\n");

    HAL_GPIO_WritePin(RST_ETH_GPIO_Port, RST_ETH_Pin, GPIO_PIN_RESET);
    osDelay(10);
    HAL_GPIO_WritePin(RST_ETH_GPIO_Port, RST_ETH_Pin, GPIO_PIN_SET);
    osDelay(200);

    reg_wizchip_cs_cbfunc(W5500_Select, W5500_Deselect);
    reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);


    uint8_t tx_mem_size[8] = {2, 2, 2, 2, 2, 2, 2, 2};
    uint8_t rx_mem_size[8] = {2, 2, 2, 2, 2, 2, 2, 2};

    if (wizchip_init(tx_mem_size, rx_mem_size) < 0) {
        printf("Error, Ethernet not responding.\r\n");
        Error_Handler();
        return -1;
    }

    // Establish Static IP
    wiz_NetInfo netInfo = {
        .mac = {0x00, 0x08, 0xDC, 0xAB, 0xCD, 0xEF}, // MAC
        .ip  = {10, 86, 110, 200},                   // Flight Segment IP
        .sn  = {255, 255, 255, 0},                   // Netmask
        .gw  = {10, 86, 110, 190},                   // Default gateway
        .dhcp = NETINFO_STATIC
    };

    uint8_t version = getVERSIONR();
    printf("W5500 Version Register: 0x%02X\r\n", version);

    wizchip_setnetinfo(&netInfo);

    wiz_NetInfo verifyInfo;
    wizchip_getnetinfo(&verifyInfo);
    printf("W5500 initialized.\r\n");
    printf(" -> IP: %d.%d.%d.%d\r\n", verifyInfo.ip[0], verifyInfo.ip[1], verifyInfo.ip[2], verifyInfo.ip[3]);
    printf(" -> Gateway: %d.%d.%d.%d\r\n", verifyInfo.gw[0], verifyInfo.gw[1], verifyInfo.gw[2], verifyInfo.gw[3]);

    return 1;
}
