/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    iwdg.h
  * @brief   This file contains all the function prototypes for
  *          the iwdg.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __IWDG_H__
#define __IWDG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include "cmsis_os2.h"
/* USER CODE END Includes */

extern IWDG_HandleTypeDef hiwdg;

/* USER CODE BEGIN Private defines */
typedef struct {
    const osThreadId_t* taskHandle;
    uint8_t isTaskActive;
} TaskInfo;

typedef enum {
    DEFAULT_TASK,
    CAN_RX_TASK,
    CAN_TX_TASK,
    READ_ADC_TASK,
    READ_TEMP_TASK,
    READ_SHOCKS_TASK,
    READ_FLOW_TASK,
    READ_SPEED_TASK,
    IMU_CAN_PROC_TASK,
    WATCH_DOG_TASK,
    NUM_TASKS
} TaskBit_t;

bool startFromIWDG();
extern osEventFlagsId_t iwdgEventGroupHandle;
void kickWatchdogBit(osThreadId_t taskHandle);
/* USER CODE END Private defines */

void MX_IWDG_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __IWDG_H__ */

