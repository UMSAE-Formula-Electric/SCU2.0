/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CAN_TX_TASK_ENABLED 1
#define READ_TEMP_TASK_ENABLED 1
#define IWDG_RELOAD_PERIOD 4094
#define READ_SPEEDS_TASK_ENABLED 1
#define IWDG_EVENT_ALL_ALLOWED_BITS 0xFFFFFF
#define IMU_CAN_PROC_TASK_ENABLED 1
#define READ_FLOW_TASK_ENABLED 1
#define CAN_RX_TASK_ENABLED 1
#define DEFAULT_TASK_ENABLED 0
#define READ_SHOCKS_TASK_ENABLED 1
#define WATCH_DOG_TASK_ENABLED 1
#define VBATT_Pin GPIO_PIN_7
#define VBATT_GPIO_Port GPIOA
#define DBG_LED1_BLUE_Pin GPIO_PIN_8
#define DBG_LED1_BLUE_GPIO_Port GPIOE
#define DBG_LED1_GREEN_Pin GPIO_PIN_9
#define DBG_LED1_GREEN_GPIO_Port GPIOE
#define DBG_LED1_RED_Pin GPIO_PIN_10
#define DBG_LED1_RED_GPIO_Port GPIOE
#define RR_WHEEL_S_Pin GPIO_PIN_0
#define RR_WHEEL_S_GPIO_Port GPIOD
#define RL_WHEEL_S_Pin GPIO_PIN_1
#define RL_WHEEL_S_GPIO_Port GPIOD
#define FR_WHEEL_S_Pin GPIO_PIN_3
#define FR_WHEEL_S_GPIO_Port GPIOD
#define FL_WHEEL_S_Pin GPIO_PIN_4
#define FL_WHEEL_S_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
