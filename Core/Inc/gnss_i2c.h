/*
 * gnss_i2c.h
 *
 * I2C transport for the u-blox MAX-M10S (SparkFun GPS-18037).
 */

#ifndef INC_GNSS_I2C_H_
#define INC_GNSS_I2C_H_

#include "main.h"

#define GNSS_I2C_ADDRESS          (0x42U << 1)
#define GNSS_I2C_REG_AVAILABLE    0xFD
#define GNSS_I2C_REG_DATA_STREAM  0xFF

HAL_StatusTypeDef GNSS_I2C_IsReady(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef GNSS_I2C_Read(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t capacity, uint16_t *bytes_read);

/* Caller serializes each I2C operation. Zero-initialize/reset parser per attempt. */
typedef struct {
    uint32_t index;
    uint16_t length;
    uint8_t header[4];
    uint8_t payload[9];
    uint8_t ck_a, ck_b, received_ck_a;
} GNSS_UBXParser;

HAL_StatusTypeDef GNSS_I2C_SetAirborne(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef GNSS_I2C_RequestDynamicModel(I2C_HandleTypeDef *hi2c);
/* Returns 1 only for a checksum-valid RAM VALGET response with model 6. */
uint8_t GNSS_I2C_AirborneVerified(GNSS_UBXParser *parser, uint8_t byte);

#endif /* INC_GNSS_I2C_H_ */
