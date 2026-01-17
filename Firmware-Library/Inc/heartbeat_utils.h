/*
 * heartbeat_utils.h
 *
 *  Created on: May 1, 2024
 *      Author: tonyz
 */

#ifndef INC_HEARTBEAT_UTILS_H_
#define INC_HEARTBEAT_UTILS_H_


#include "cmsis_os2.h"
#include "task.h"
#include "FreeRTOS.h"
#include <stdbool.h>

typedef enum {
	HEARTBEAT_NONE = 0, //have not received anything from ACB/MC yet
	HEARTBEAT_LOST, //similar to none except that we previously had acb/mc signal and lost it
	HEARTBEAT_PRESENT
} HeartbeatState_t;

typedef enum {
	HEARTBEAT_REQUEST_NOTIFY = 0,
	HEARTBEAT_RESPONSE_NOTIFY
} HeartbeatNotify_t;
#endif /* INC_HEARTBEAT_UTILS_H_ */
