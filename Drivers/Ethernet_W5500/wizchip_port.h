/*
 * wizchip_port.h
 *
 *  Created on: 3 jul 2026
 *      Author: Mario
 */

#ifndef ETHERNET_W5500_WIZCHIP_PORT_H_
#define ETHERNET_W5500_WIZCHIP_PORT_H_

#include "main.h"

#define GS_IP {172, 16, 18, 150}
#define GS2_IP {172, 16, 18, 151}
#define GS_PORT 5000

/**
 * @brief Inicializa el hardware del W5500 (Reset físico), registra los callbacks
 * de SPI con la ioLibrary de Wiznet y configura la IP estática.
 */
int8_t W5500_Init(void);

/**
 * @brief Callbacks físicos que exige la librería de Wiznet para controlar el bus.
 * Se exponen aquí por si en algún test necesitáis forzar el CS a mano.
 */
void W5500_Select(void);
void W5500_Deselect(void);
uint8_t W5500_ReadByte(void);
void W5500_WriteByte(uint8_t wb);

/* Atomic W5500 VDM transfers. These functions control CS internally. */
int8_t W5500_SPI_ReadFrame(uint32_t addr_sel, uint8_t *data, uint16_t length);
int8_t W5500_SPI_WriteFrame(uint32_t addr_sel, const uint8_t *data, uint16_t length);


#endif /* ETHERNET_W5500_WIZCHIP_PORT_H_ */
