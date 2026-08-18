/*
 * gnss_i2c.c
 *
 * I2C transport for the u-blox MAX-M10S (SparkFun GPS-18037).
 */

#include "gnss_i2c.h"

#define GNSS_I2C_TIMEOUT_MS 100U

HAL_StatusTypeDef GNSS_I2C_IsReady(I2C_HandleTypeDef *hi2c)
{
    return HAL_I2C_IsDeviceReady(hi2c, GNSS_I2C_ADDRESS, 2, GNSS_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef GNSS_I2C_Read(I2C_HandleTypeDef *hi2c,
                                uint8_t *data,
                                uint16_t capacity,
                                uint16_t *bytes_read)
{
    uint8_t available_bytes[2];
    uint16_t available;
    HAL_StatusTypeDef status;

    if ((hi2c == NULL) || (data == NULL) || (bytes_read == NULL) || (capacity == 0U)) {
        return HAL_ERROR;
    }

    *bytes_read = 0U;

    status = HAL_I2C_Mem_Read(hi2c,
                              GNSS_I2C_ADDRESS,
                              GNSS_I2C_REG_AVAILABLE,
                              I2C_MEMADD_SIZE_8BIT,
                              available_bytes,
                              sizeof(available_bytes),
                              GNSS_I2C_TIMEOUT_MS);
    if (status != HAL_OK) {
        return status;
    }

    available = ((uint16_t)available_bytes[0] << 8) | available_bytes[1];
    if (available == 0U) {
        return HAL_OK;
    }

    if (available > capacity) {
        available = capacity;
    }

    /* Reading 0xFD and 0xFE advances the MAX-M10S register pointer to 0xFF.
     * A current-address read now returns the queued NMEA/UBX stream bytes. */
    status = HAL_I2C_Master_Receive(hi2c,
                                    GNSS_I2C_ADDRESS,
                                    data,
                                    available,
                                    GNSS_I2C_TIMEOUT_MS);
    if (status == HAL_OK) {
        *bytes_read = available;
    }

    return status;
}
