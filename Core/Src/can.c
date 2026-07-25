/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
#include "can.h"

/* USER CODE BEGIN 0 */
#include <string.h>
#include "logger.h"
#include "imu.h"
#include "cmsis_os2.h"
#include "iwdg.h"

/* How long StartCanTxTask waits for work before looping to kick the watchdog */
#define CAN_TX_QUEUE_WAIT_MS   50
/* How long to wait for one of the 3 hardware mailboxes before giving up */
#define CAN_TX_MAILBOX_WAIT_MS 5

uint32_t TxMailbox;

/* USER CODE END 0 */

CAN_HandleTypeDef hcan2;

/* CAN2 init function */
void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */
    CAN_FilterTypeDef sFilterConfig;

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 2;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */
    sFilterConfig.FilterBank = 14;	//CAN2 uses FilterBank values 14 through 27.
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    if(HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) != HAL_OK)
    {
        /* Filter configuration Error */
        Error_Handler();
    }

  /* USER CODE END CAN2_Init 2 */

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN2)
  {
  /* USER CODE BEGIN CAN2_MspInit 0 */

  /* USER CODE END CAN2_MspInit 0 */
    /* CAN2 clock enable */
    __HAL_RCC_CAN2_CLK_ENABLE();
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN2 GPIO Configuration
    PB5     ------> CAN2_RX
    PB6     ------> CAN2_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN2 interrupt Init */
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN2_SCE_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN2_SCE_IRQn);
  /* USER CODE BEGIN CAN2_MspInit 1 */

  /* USER CODE END CAN2_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN2)
  {
  /* USER CODE BEGIN CAN2_MspDeInit 0 */

  /* USER CODE END CAN2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN2_CLK_DISABLE();
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN2 GPIO Configuration
    PB5     ------> CAN2_RX
    PB6     ------> CAN2_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5|GPIO_PIN_6);

    /* CAN2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN2_SCE_IRQn);
  /* USER CODE BEGIN CAN2_MspDeInit 1 */

  /* USER CODE END CAN2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void StartCanTxTask(void *argument){
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

    CAN_TxPacketTypeDef txPacket;
    osStatus_t isMsgTakenFromQueue;

    for(;;){
        kickWatchdogBit(osThreadGetId());

        /* Block rather than poll. The old spin left this task permanently
         * ready at osPriorityNormal, which starved every lower priority task
         * and burned the CPU that the sensor tasks needed. Waking on a timeout
         * keeps the watchdog bit set even when there is no CAN traffic. */
        isMsgTakenFromQueue = osMessageQueueGet(canTxPacketQueueHandle, &txPacket, 0, CAN_TX_QUEUE_WAIT_MS);
        if (isMsgTakenFromQueue != osOK) {
            continue;
        }

        /* The packet is already off the queue, so a full mailbox set would
         * destroy it. Wait for a mailbox instead of dropping the frame. */
        uint32_t mailboxWaitMs = CAN_TX_MAILBOX_WAIT_MS;
        while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) == 0 && mailboxWaitMs > 0) {
            osDelay(1);
            mailboxWaitMs--;
        }

        if (HAL_CAN_AddTxMessage(&hcan2, &(txPacket.txPacketHeader), txPacket.txPacketData, &TxMailbox) != HAL_OK) {
            logMessage("SCU couldn't send a message to the CAN Bus.\r\n", true);
        }
    }
}
void convertDoubleToCAN(double *value, uint8_t *canData){
    uint16_t valueScaled[4];

    for(int i = 0; i < 4; i++)
    {
        valueScaled[i] = value[i]*100;//scale by 100 based on dbc
    }

    canData[0] = valueScaled[0] & 0xFF;
    canData[1] = valueScaled[0] >> 8;

    canData[2] = valueScaled[1] & 0xFF;
    canData[3] = valueScaled[1] >> 8;

    canData[4] = valueScaled[2] & 0xFF;
    canData[5] = valueScaled[2] >> 8;

    canData[6] = valueScaled[3] & 0xFF;
    canData[7] = valueScaled[3] >> 8;

}
void convertFlowrateToCAN(double value, uint8_t *canData){
	uint16_t valueScaled;

    valueScaled = value*100;//scale by 100 based on dbc

    canData[0] = valueScaled & 0xFF;
    canData[1] = valueScaled >> 8;
}
/* USER CODE END 1 */
