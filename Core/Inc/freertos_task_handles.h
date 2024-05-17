//
// Created by tonyz on 5/14/2024.
//

#ifndef FREERTOS_TASK_HANDLES_H
#define FREERTOS_TASK_HANDLES_H

#include "cmsis_os2.h"

extern osThreadId_t defaultTaskHandle;
extern osThreadId_t canRxTaskHandle;
extern osThreadId_t canTxTaskHandle;
extern osThreadId_t readAdcTaskHandle;
extern osThreadId_t readTempTaskHandle;
extern osThreadId_t readShocksTaskHandle;
extern osThreadId_t readFlowTaskHandle;
extern osThreadId_t readSpeedsTaskHandle;
extern osThreadId_t imuCanProcTaskHandle;
extern osThreadId_t watchDogTaskHandle;

#endif //FREERTOS_TASK_HANDLES_H
