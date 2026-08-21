/*
 * mission_time.c
 */

#include "mission_time.h"

#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

static uint32_t mission_time_origin_tick = 0U;
static uint8_t mission_time_synchronized = 0U;

void MissionTime_Sync(void)
{
    taskENTER_CRITICAL();
    mission_time_origin_tick = HAL_GetTick();
    mission_time_synchronized = 1U;
    taskEXIT_CRITICAL();
}

uint32_t MissionTime_GetMs(void)
{
    uint32_t now;
    uint32_t origin;
    uint8_t synchronized;

    taskENTER_CRITICAL();
    now = HAL_GetTick();
    origin = mission_time_origin_tick;
    synchronized = mission_time_synchronized;
    taskEXIT_CRITICAL();

    /* Unsigned subtraction also preserves correct elapsed time across one
     * HAL tick wrap. Before synchronization, retain the legacy boot time. */
    return (synchronized != 0U) ? (now - origin) : now;
}
