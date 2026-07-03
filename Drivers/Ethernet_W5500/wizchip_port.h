/*
 * wizchip_port.h
 *
 *  Created on: 3 jul 2026
 *      Author: Mario
 */

#ifndef ETHERNET_W5500_WIZCHIP_PORT_H_
#define ETHERNET_W5500_WIZCHIP_PORT_H_

#include "main.h"

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


#endif /* ETHERNET_W5500_WIZCHIP_PORT_H_ */
