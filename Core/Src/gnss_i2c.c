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

/* UBX-CFG-VALSET / VALGET, CFG-NAVSPG-DYNMODEL (0x20110021).
 * Write UBX directly: I2C writes have no register-address prefix.
 * RAM only: reapply on every MCU boot; do not depend on backup power.
 */
static HAL_StatusTypeDef GNSS_I2C_SendConfig(I2C_HandleTypeDef *hi2c,
                                           uint8_t id, uint8_t set)
{
    uint8_t packet[17] = {0xB5, 0x62, 0x06, id, 0, 0,
                         0, set, 0, 0, 0x21, 0x00, 0x11, 0x20, 6, 0, 0};
    uint16_t size = set ? 17U : 16U;
    uint8_t ck_a = 0U, ck_b = 0U;
    if (hi2c == NULL) {
        return HAL_ERROR;
    }
    packet[4] = set ? 9U : 8U;
    for (uint16_t i = 2U; i < size - 2U; ++i) {
        ck_a = (uint8_t)(ck_a + packet[i]);
        ck_b = (uint8_t)(ck_b + ck_a);
    }
    packet[size - 2U] = ck_a;
    packet[size - 1U] = ck_b;
    return HAL_I2C_Master_Transmit(hi2c, GNSS_I2C_ADDRESS, packet,
                                   size, GNSS_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef GNSS_I2C_SetAirborne(I2C_HandleTypeDef *hi2c)
{
    return GNSS_I2C_SendConfig(hi2c, 0x8AU, 1U);
}

HAL_StatusTypeDef GNSS_I2C_RequestDynamicModel(I2C_HandleTypeDef *hi2c)
{
    return GNSS_I2C_SendConfig(hi2c, 0x8BU, 0U);
}

uint8_t GNSS_I2C_AirborneVerified(GNSS_UBXParser *p, uint8_t byte)
{
    if (p == NULL) {
        return 0U;
    }
    if (p->index == 0U) {
        if (byte == 0xB5U) { p->index = 1U; }
        return 0U;
    }
    if (p->index == 1U) {
        p->index = (byte == 0x62U) ? 2U : ((byte == 0xB5U) ? 1U : 0U);
        p->ck_a = 0U;
        p->ck_b = 0U;
        return 0U;
    }
    if (p->index < 6U) {
        p->header[p->index - 2U] = byte;
        if (p->index == 5U) {
            p->length = (uint16_t)p->header[2] | ((uint16_t)byte << 8);
        }
    } else if (p->index < (uint32_t)p->length + 6U) {
        /* Skip unrelated UBX payloads without buffering or resyncing inside them. */
        if (p->index - 6U < sizeof(p->payload)) {
            p->payload[p->index - 6U] = byte;
        }
    } else if (p->index == (uint32_t)p->length + 6U) {
        p->received_ck_a = byte;
        ++p->index;
        return 0U;
    } else {
        uint8_t verified = (p->received_ck_a == p->ck_a && byte == p->ck_b &&
            p->header[0] == 0x06U && p->header[1] == 0x8BU && p->length == 9U &&
            p->payload[0] == 1U && p->payload[1] == 0U &&
            p->payload[2] == 0U && p->payload[3] == 0U &&
            p->payload[4] == 0x21U && p->payload[5] == 0U &&
            p->payload[6] == 0x11U && p->payload[7] == 0x20U &&
            p->payload[8] == 6U);
        p->index = 0U;
        return verified;
    }
    p->ck_a = (uint8_t)(p->ck_a + byte);
    p->ck_b = (uint8_t)(p->ck_b + p->ck_a);
    ++p->index;
    return 0U;
}
