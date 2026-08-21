/*
 * mission_time.h
 *
 * Common mission elapsed-time reference. This module is intentionally
 * independent from the telemetry transports.
 */

#ifndef INC_MISSION_TIME_H_
#define INC_MISSION_TIME_H_

#include <stdint.h>

/* Before the first synchronization this returns milliseconds since boot.
 * After MissionTime_Sync(), it returns milliseconds since the latest sync. */
uint32_t MissionTime_GetMs(void);

/* Sets the current instant as mission T+0. Safe to call again to resync. */
void MissionTime_Sync(void);

#endif /* INC_MISSION_TIME_H_ */
