/*
 * w25log.c
 *
 *  Created on: 30 jun 2026
 *      Author: Mario
 */


#include "w25log.h"
#include <string.h>

#define DUMP_START_MARKER 0xDEADBEEF
#define DUMP_END_MARKER   0xCAFEBABE
#define W25LOG_BOOT_ID_OFFSET (4U + W25LOG_PACKET_SIZE)
#define W25LOG_RESET_CAUSE_OFFSET (W25LOG_BOOT_ID_OFFSET + sizeof(uint32_t))

static uint8_t log_io_ok = 1U;

static uint8_t slot_is_written(const uint8_t *slot_header4) {
    return (slot_header4[0] == W25LOG_MAGIC0 &&
            slot_header4[1] == W25LOG_MAGIC1 &&
            slot_header4[2] == W25LOG_MAGIC2 &&
            slot_header4[3] == W25LOG_MAGIC3);
}

static uint8_t sector_is_erased(uint32_t sector_index) {
    uint8_t header[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    uint32_t addr = sector_index * W25LOG_SECTOR_SIZE;
    if (W25QXX_ReadData(addr, header, 4U) == 0U) {
        log_io_ok = 0U;
        return 0U;
    }
    return !slot_is_written(header);
}

static uint32_t find_first_erased_sector(void) {
    uint32_t lo = 0, hi = W25LOG_TOTAL_SECTORS;

    if (sector_is_erased(0)) {
        return 0;
    }
    if (!sector_is_erased(W25LOG_TOTAL_SECTORS - 1)) {
        return W25LOG_TOTAL_SECTORS;
    }

    while (hi - lo > 1) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (sector_is_erased(mid)) {
            hi = mid;
        } else {
            lo = mid;
        }
    }
    return hi;
}

static uint32_t find_first_free_slot_in_sector(uint32_t sector_index) {
    uint8_t header[4];
    uint32_t base = sector_index * W25LOG_SECTOR_SIZE;

    for (uint32_t s = 0; s < W25LOG_SLOTS_PER_SECTOR; s++) {
        uint32_t addr = base + s * W25LOG_SLOT_SIZE;
        if (W25QXX_ReadData(addr, header, 4U) == 0U) {
            log_io_ok = 0U;
            return base + W25LOG_SECTOR_SIZE;
        }
        if (!slot_is_written(header)) {
            return addr;
        }
    }

    return base + W25LOG_SECTOR_SIZE;
}

static uint8_t W25LOG_StartNewBoot(W25LOG_State_t *state) {
    uint32_t previous_boot_id = 0xFFFFFFFFU;

    if (state->packet_count == 0U) {
        state->boot_id = 1U;
        return 1U;
    }

    uint32_t last_slot = state->next_write_addr - W25LOG_SLOT_SIZE;
    if (W25QXX_ReadData(last_slot + W25LOG_BOOT_ID_OFFSET,
                        (uint8_t *)&previous_boot_id,
                        sizeof(previous_boot_id)) == 0U) {
        return 0U;
    }

    /* Old slots created before boot IDs contain 0xFFFFFFFF in the padding. */
    state->boot_id = (previous_boot_id == 0xFFFFFFFFU)
        ? 1U
        : previous_boot_id + 1U;
    return 1U;
}

uint8_t W25LOG_Init(W25LOG_State_t *state) {
    log_io_ok = 1U;
    uint32_t erased_sector = find_first_erased_sector();

    if (log_io_ok == 0U) {
        return 0U;
    }

    if (erased_sector == 0) {
        state->next_write_addr = 0;
        state->packet_count = 0;
        return W25LOG_StartNewBoot(state);
    }

    if (erased_sector == W25LOG_TOTAL_SECTORS) {
        state->next_write_addr = W25LOG_FLASH_SIZE;
        state->packet_count = W25LOG_TOTAL_SLOTS;
        return W25LOG_StartNewBoot(state);
    }

    uint32_t last_partial_sector = erased_sector - 1;
    uint32_t write_addr = find_first_free_slot_in_sector(last_partial_sector);

    if (write_addr >= (last_partial_sector + 1) * W25LOG_SECTOR_SIZE) {
        /* The previous sector ended exactly on its last slot. Continue at the
         * first slot of the already-erased sector instead of reporting error. */
        state->next_write_addr = write_addr;
        state->packet_count = write_addr / W25LOG_SLOT_SIZE;
        return W25LOG_StartNewBoot(state);
    }

    state->next_write_addr = write_addr;
    state->packet_count = write_addr / W25LOG_SLOT_SIZE;
    return W25LOG_StartNewBoot(state);
}

uint8_t W25LOG_Format(W25LOG_State_t *state) {
    for (uint32_t s = 0; s < W25LOG_TOTAL_SECTORS; s++) {
        if (W25QXX_SectorErase(s * W25LOG_SECTOR_SIZE) == 0U) {
            return 0U;
        }
    }
    state->next_write_addr = 0;
    state->packet_count = 0;
    state->boot_id = 1U;
    return 1U;
}

uint8_t W25LOG_WritePacket(W25LOG_State_t *state, const uint8_t *packet_data) {
    if (state->next_write_addr >= W25LOG_FLASH_SIZE) {
        return 1;
    }

    if ((state->next_write_addr % W25LOG_SECTOR_SIZE) == 0) {
        if (W25QXX_SectorErase(state->next_write_addr) == 0U) {
            return 2U;
        }
    }

    uint8_t slot_buf[W25LOG_SLOT_SIZE];
    memset(slot_buf, 0xFF, sizeof(slot_buf));
    slot_buf[0] = W25LOG_MAGIC0;
    slot_buf[1] = W25LOG_MAGIC1;
    slot_buf[2] = W25LOG_MAGIC2;
    slot_buf[3] = W25LOG_MAGIC3;
    memcpy(&slot_buf[4], packet_data, W25LOG_PACKET_SIZE);
    memcpy(&slot_buf[W25LOG_BOOT_ID_OFFSET], &state->boot_id,
           sizeof(state->boot_id));
    slot_buf[W25LOG_RESET_CAUSE_OFFSET] = state->reset_cause;

    if (W25QXX_PageProgram(state->next_write_addr, slot_buf,
                           W25LOG_SLOT_SIZE) == 0U) {
        return 2U;
    }

    state->next_write_addr += W25LOG_SLOT_SIZE;
    state->packet_count++;
    return 0;
}

uint8_t W25LOG_ReadLastPacket(W25LOG_State_t *state, uint8_t *out_buf) {
    if (state->packet_count == 0) {
        return 1;
    }

    uint32_t last_addr = state->next_write_addr - W25LOG_SLOT_SIZE;
    uint8_t slot_buf[W25LOG_SLOT_SIZE];
    if (W25QXX_ReadData(last_addr, slot_buf, W25LOG_SLOT_SIZE) == 0U) {
        return 1U;
    }

    memcpy(out_buf, &slot_buf[4], W25LOG_PACKET_SIZE);
    return 0;
}

void W25LOG_ReadAll(W25LOG_State_t *state, W25LOG_ReadCallback cb, void *ctx) {
    uint8_t slot_buf[W25LOG_SLOT_SIZE];
    uint32_t total_slots = state->next_write_addr / W25LOG_SLOT_SIZE;

    for (uint32_t i = 0; i < total_slots; i++) {
        uint32_t addr = i * W25LOG_SLOT_SIZE;
        if (W25QXX_ReadData(addr, slot_buf, W25LOG_SLOT_SIZE) == 0U) {
            return;
        }

        if (slot_is_written(slot_buf)) {
            cb(i, &slot_buf[4], ctx);
        }
    }
}


void W25LOG_DumpToUART(UART_HandleTypeDef *huart, W25LOG_State_t *state) {
    uint32_t marker;

    marker = DUMP_START_MARKER;
    HAL_UART_Transmit(huart, (uint8_t*)&marker, 4, 100);

    HAL_UART_Transmit(huart, (uint8_t*)&state->packet_count, 4, 100);

    uint8_t slot_buf[W25LOG_SLOT_SIZE];
    uint32_t total_slots = state->next_write_addr / W25LOG_SLOT_SIZE;

    for (uint32_t i = 0; i < total_slots; i++) {
        uint32_t addr = i * W25LOG_SLOT_SIZE;
        if (W25QXX_ReadData(addr, slot_buf, W25LOG_SLOT_SIZE) == 0U) {
            break;
        }

        if (slot_buf[0] == W25LOG_MAGIC0 && slot_buf[1] == W25LOG_MAGIC1 &&
            slot_buf[2] == W25LOG_MAGIC2 && slot_buf[3] == W25LOG_MAGIC3) {
            HAL_UART_Transmit(huart, &slot_buf[4], W25LOG_PACKET_SIZE, 100);
        }
    }

    marker = DUMP_END_MARKER;
    HAL_UART_Transmit(huart, (uint8_t*)&marker, 4, 100);
}
