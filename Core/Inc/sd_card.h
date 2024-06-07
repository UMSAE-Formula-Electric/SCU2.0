/*
 * sd_card2.h
 *
 *  Created on: Mar 3, 2024
 *      Author: niko
 */

#ifndef INC_SD_CARD_H_
#define INC_SD_CARD_H_

#include "fatfs.h"

// if this is defined a benchmark test will be created and done, disable this in the final thing
// #define DO_SD_CARD_BENCHMARK
/*
FRESULT sd_init(void);
FRESULT sd_mount(void);
FRESULT sd_open_log_file(void);
FRESULT sd_log_to_file(char*, UINT);
FRESULT sd_switch_log(void);
FRESULT sd_eject(void);
*/
// How many writes are allowed to be buffered before we force them to be written to a file
#define WRITES_UNTIL_SYNC 100

#define SD_REQUEST_MAX_MESSAGE_LENGTH 64

#define SD_QUEUE_SIZE sizeof(SDRequest)
#define SD_QUEUE_LENGTH 128

typedef enum {
	SDREQUEST_WRITE,
	SDREQUEST_SYNC,
} SDRequestType;

typedef struct {
	SDRequestType type;
	uint32_t length;
	char message[SD_REQUEST_MAX_MESSAGE_LENGTH];
} SDRequest;
extern osMessageQueueId_t sdCardQueueHandle;


_Bool SDCardWrite(char *message, uint32_t length);
_Bool SDCardSync(void);
void SD_Init(void);

#endif /* INC_SD_CARD_H_ */
