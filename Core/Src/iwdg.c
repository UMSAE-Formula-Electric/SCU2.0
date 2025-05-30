/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    iwdg.c
  * @brief   This file provides code for the configuration
  *          of the IWDG instances.
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
#include "iwdg.h"

/* USER CODE BEGIN 0 */
#include "FreeRTOS.h"
#include "freertos_task_handles.h"
#include "event_groups.h"

/* USER CODE END 0 */

IWDG_HandleTypeDef hiwdg;

/* IWDG init function */
void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
  hiwdg.Init.Reload = IWDG_RELOAD_PERIOD;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/* USER CODE BEGIN 1 */
/**
 * @brief Checks if the system has resumed from IWDG reset
 *
 * @return true if the system has resumed from IWDG reset
 * @return false if the system has not resumed from IWDG reset
 */
bool startFromIWDG() {

    bool startFromWD = false; //true if we started from watchdog reset

    /* Check if the system has resumed from IWDG reset */
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET) {

        startFromWD = true;

        /* Clear reset flags */
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }

    return startFromWD;
}

/**
 * @brief Gets a static list of all the FreeRTOS tasks' attributes and their activity status
 *
 * @param count The number of tasks
 * @return TaskInfo* The array of the tasks' information
 */
TaskInfo* getTaskInfos(size_t* count) {
    static TaskInfo taskInfos[] = {
            {&defaultTaskHandle, DEFAULT_TASK_ENABLED},
            {&canTxTaskHandle, CAN_TX_TASK_ENABLED},
            {&canRxTaskHandle, CAN_RX_TASK_ENABLED},
            {&readTempTaskHandle, READ_TEMP_TASK_ENABLED},
            {&readShocksTaskHandle, READ_SHOCKS_TASK_ENABLED},
            {&readFlowTaskHandle, READ_FLOW_TASK_ENABLED},
            {&readSpeedsTaskHandle, READ_SPEEDS_TASK_ENABLED},
            {&imuCanProcTaskHandle, IMU_CAN_PROC_TASK_ENABLED},
            {&watchDogTaskHandle, WATCH_DOG_TASK_ENABLED},
    };

    *count = sizeof(taskInfos) / sizeof(TaskInfo);
    return taskInfos;
}

/**
 * @brief Gets the bit position of a task in the iwdgEventGroup bits
 *
 * @param taskHandle The handle of the task
 * @return TaskBit_t The bit position of the task
 */
TaskBit_t getTaskBit(osThreadId_t taskHandle) {
    size_t taskCount;
    TaskInfo* taskInfos = getTaskInfos(&taskCount);
    for (TaskBit_t taskBit = 0; taskBit < taskCount; taskBit++) {
        if (taskHandle == *taskInfos[taskBit].taskHandle) {
            return taskBit;
        }
    }
    return NUM_TASKS;
}

/**
 * @brief Sets the bit of a task in the iwdgEventGroup bits
 *
 * @param taskHandle The handle of the task
 */
void kickWatchdogBit(osThreadId_t taskHandle) {
    TaskBit_t bitPosition = getTaskBit(taskHandle);
    xEventGroupSetBits(iwdgEventGroupHandle, (1 << bitPosition));
}

/**
 * @brief Checks if a task is active
 *
 * @param taskBit The bit position of the task in the taskInfos array
 * @param taskInfos The array of the tasks' information
 * @param taskCount The number of tasks
 * @return true if the task is active
 * @return false if the task is not active
 */
bool isTaskActive(TaskBit_t taskBit, TaskInfo *taskInfos, size_t taskCount) {
    if (taskBit < taskCount) {
        return taskInfos[taskBit].isTaskActive == 1;
    }

    return false;
}

/**
 * @brief Checks if a task is ready
 *
 * @param bitPosition The bit position of the task in the iwdgEventGroup bits
 * @param taskBits The bits of the iwdgEventGroup
 * @return true if the task is ready
 * @return false if the task is not ready
 */
bool isTaskReady(TaskBit_t bitPosition, EventBits_t taskBits) {
    return (taskBits & (1 << bitPosition)) != 0;
}

/**
 * @brief Checks if all active tasks have set their bits in iwdgEventGroup
 *
 * @return true if all active tasks have set their bits in iwdgEventGroup
 * @return false if not all active tasks have set their bits in iwdgEventGroup
 */
bool areAllActiveTasksReady() {
    size_t taskCount;
    TaskBit_t currTaskBit;
    osThreadId_t currTask;
    TaskInfo* taskInfos = getTaskInfos(&taskCount);
    EventBits_t taskBits = xEventGroupGetBits(iwdgEventGroupHandle);

    for (int taskBit = 0; taskBit < taskCount; taskBit++) {
        currTask = *taskInfos[taskBit].taskHandle;
        currTaskBit = getTaskBit(currTask);
        if (isTaskActive(currTaskBit, taskInfos, taskCount) && !isTaskReady(currTaskBit, taskBits)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief The watchdog FreeRTOS task
 *
 * @param argument the TASK_ENABLED value of the task used in freertos.c
 */
void StartWatchDogTask(void *argument) {
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

    MX_IWDG_Init();

    bool allActiveTasksReady;

    for(;;) {
        kickWatchdogBit(osThreadGetId());
        allActiveTasksReady = areAllActiveTasksReady();                               // Check if all active tasks have set their bits in iwdgEventGroup
        if (allActiveTasksReady) {                                                    // If all tasks have set their bits before IWDG_RELOAD_PERIOD, refresh the watchdog timer
            HAL_IWDG_Refresh(&hiwdg);
            xEventGroupClearBits(iwdgEventGroupHandle, IWDG_EVENT_ALL_ALLOWED_BITS);
        }

        osDelay(IWDG_RELOAD_PERIOD / 2);                                       // Delay for IWDG_RELOAD_PERIOD
    }
}
/* USER CODE END 1 */
