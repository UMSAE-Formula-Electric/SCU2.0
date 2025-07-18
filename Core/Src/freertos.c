/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for canRxTask */
osThreadId_t canRxTaskHandle;
const osThreadAttr_t canRxTask_attributes = {
  .name = "canRxTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for canTxTask */
osThreadId_t canTxTaskHandle;
const osThreadAttr_t canTxTask_attributes = {
  .name = "canTxTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for readTempTask */
osThreadId_t readTempTaskHandle;
const osThreadAttr_t readTempTask_attributes = {
  .name = "readTempTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for readShocksTask */
osThreadId_t readShocksTaskHandle;
const osThreadAttr_t readShocksTask_attributes = {
  .name = "readShocksTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for readFlowTask */
osThreadId_t readFlowTaskHandle;
const osThreadAttr_t readFlowTask_attributes = {
  .name = "readFlowTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for readSpeedsTask */
osThreadId_t readSpeedsTaskHandle;
const osThreadAttr_t readSpeedsTask_attributes = {
  .name = "readSpeedsTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for imuCanProcTask */
osThreadId_t imuCanProcTaskHandle;
const osThreadAttr_t imuCanProcTask_attributes = {
  .name = "imuCanProcTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for watchDogTask */
osThreadId_t watchDogTaskHandle;
const osThreadAttr_t watchDogTask_attributes = {
  .name = "watchDogTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for canRxPacketQueue */
osMessageQueueId_t canRxPacketQueueHandle;
const osMessageQueueAttr_t canRxPacketQueue_attributes = {
  .name = "canRxPacketQueue"
};
/* Definitions for canTxPacketQueue */
osMessageQueueId_t canTxPacketQueueHandle;
const osMessageQueueAttr_t canTxPacketQueue_attributes = {
  .name = "canTxPacketQueue"
};
/* Definitions for iwdgEventGroup */
osEventFlagsId_t iwdgEventGroupHandle;
const osEventFlagsAttr_t iwdgEventGroup_attributes = {
  .name = "iwdgEventGroup"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
extern void StartCanRxTask(void *argument);
extern void StartCanTxTask(void *argument);
extern void StartReadTempTask(void *argument);
extern void StartReadShocksTask(void *argument);
extern void StartReadFlowTask(void *argument);
extern void StartReadSpeedsTask(void *argument);
extern void StartImuCanProcTask(void *argument);
extern void StartWatchDogTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of canRxPacketQueue */
  canRxPacketQueueHandle = osMessageQueueNew (32, sizeof(CAN_RxPacketTypeDef), &canRxPacketQueue_attributes);

  /* creation of canTxPacketQueue */
  canTxPacketQueueHandle = osMessageQueueNew (32, sizeof(CAN_TxPacketTypeDef), &canTxPacketQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, (void*) DEFAULT_TASK_ENABLED, &defaultTask_attributes);

  /* creation of canRxTask */
  canRxTaskHandle = osThreadNew(StartCanRxTask, (void*) CAN_RX_TASK_ENABLED, &canRxTask_attributes);

  /* creation of canTxTask */
  canTxTaskHandle = osThreadNew(StartCanTxTask, (void*) CAN_TX_TASK_ENABLED, &canTxTask_attributes);

  /* creation of readTempTask */
  readTempTaskHandle = osThreadNew(StartReadTempTask, (void*) READ_TEMP_TASK_ENABLED, &readTempTask_attributes);

  /* creation of readShocksTask */
  readShocksTaskHandle = osThreadNew(StartReadShocksTask, (void*) READ_SHOCKS_TASK_ENABLED, &readShocksTask_attributes);

  /* creation of readFlowTask */
  readFlowTaskHandle = osThreadNew(StartReadFlowTask, (void*) READ_FLOW_TASK_ENABLED, &readFlowTask_attributes);

  /* creation of readSpeedsTask */
  readSpeedsTaskHandle = osThreadNew(StartReadSpeedsTask, (void*) READ_SPEEDS_TASK_ENABLED, &readSpeedsTask_attributes);

  /* creation of imuCanProcTask */
  imuCanProcTaskHandle = osThreadNew(StartImuCanProcTask, (void*) IMU_CAN_PROC_TASK_ENABLED, &imuCanProcTask_attributes);

  /* creation of watchDogTask */
  watchDogTaskHandle = osThreadNew(StartWatchDogTask, (void*) WATCH_DOG_TASK_ENABLED, &watchDogTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of iwdgEventGroup */
  iwdgEventGroupHandle = osEventFlagsNew(&iwdgEventGroup_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    const char *msg = "Stack overflow detected in task: ";
    const char *newline = "\r\n";

    HAL_USART_Transmit(&husart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

    if (pcTaskName != NULL)
    {
        HAL_USART_Transmit(&husart2, (uint8_t *)pcTaskName, strlen(pcTaskName), HAL_MAX_DELAY);
    }
    else
    {
        const char *unknown = "Unknown";
        HAL_USART_Transmit(&husart2, (uint8_t *)unknown, strlen(unknown), HAL_MAX_DELAY);
    }

    HAL_USART_Transmit(&husart2, (uint8_t *)newline, strlen(newline), HAL_MAX_DELAY);

    taskDISABLE_INTERRUPTS();
    for(;;);
}
/* USER CODE END Application */

