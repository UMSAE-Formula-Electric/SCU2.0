/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.h
  * @brief   This file contains all the function prototypes for
  *          the can.c file
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
#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "can_utils.h"
/* USER CODE END Includes */

extern CAN_HandleTypeDef hcan2;

/* USER CODE BEGIN Private defines */
#define IMU_ACCELERATION_CAN_EXT_ID 0x8F02D80
#define IMU_ANGULAR_RATE_CAN_EXT_ID 0xCF02A80
#define CAN_ACU_CAN_ID				0x69
#define CAN_VCU_CAN_ID				0x88
#define CAN_SCU_CAN_ID				0x89
#define CAN_AMS_CAN_ID          	0x70
#define CAN_VCU_LOG_ID				0x71
/* USER CODE END Private defines */

void MX_CAN2_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H__ */

