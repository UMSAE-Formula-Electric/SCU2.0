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
#include "logger.h"

/* USER CODE BEGIN 0 */
#include <usart.h>
#include <string.h>
#include <stdio.h>
//#include "IMU.h"

CAN_RxHeaderTypeDef   RxHeader;
uint8_t               RxData[8];

CAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8];
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
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO1;
    sFilterConfig.FilterActivation = ENABLE;

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
HAL_StatusTypeDef CAN_Polling(void)
{
    uint32_t isCanRxFifoFilled = HAL_CAN_GetRxFifoFillLevel(&hcan2, CAN_RX_FIFO1);
    if (isCanRxFifoFilled < 1)
    {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef isCanMsgReceived = HAL_CAN_GetRxMessage(&hcan2, CAN_RX_FIFO1, &RxHeader, RxData);
    if (isCanMsgReceived != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

void messageReceivedFromControlUnit(const char *unitType) {
    char canMsg[50];
    if (strcmp(unitType, "VCU") == 0) strncpy(canMsg, "SCU received a CAN message from the VCU.\r\n", sizeof(canMsg) - 1);
    else if (strcmp(unitType, "ACU") == 0) strncpy(canMsg, "SCU received a CAN message from the ACU.\r\n", sizeof(canMsg) - 1);
    else if (strcmp(unitType, "SCU") == 0) strncpy(canMsg, "SCU received a CAN message from the SCU.\r\n", sizeof(canMsg) - 1);

    logMessage(canMsg, true);
}

void StartCanRxTask(void *argument)
{
//	imuState state;
    char canMsg[50];
    HAL_CAN_Start(&hcan2);

    for (;;)
    {
        if (CAN_Polling() == HAL_OK)
        {
            if (RxHeader.IDE == CAN_ID_EXT)
            {
                switch (RxHeader.ExtId)
                {
                    case IMU_ACCELERATION_CAN_EXT_ID:
//                        queueAccelerationPacket(RxData);
//						imuProcessAccelerationPacket(&state, RxData);
//						sprintf(canMsg, "IMU Acceleration Packet\r\n");
//						HAL_USART_Transmit(&husart1, (uint8_t *) canMsg, strlen(canMsg)+1, 10);
                        break;
                    case IMU_ANGULAR_RATE_CAN_EXT_ID:
//                        queueAngularRatePacket(RxData);
//						imuProcessAngularRatePacket(&state, RxData);
//						sprintf(canMsg, "IMU Angular Rate Packet\r\n");
//						HAL_USART_Transmit(&husart1, (uint8_t *) canMsg, strlen(canMsg)+1, 10);
                        break;
                }
            }
            if (RxHeader.IDE == CAN_ID_STD)
            {
                switch (RxHeader.StdId)
                {
                    case CAN_VCU_CAN_ID:
                        messageReceivedFromControlUnit("VCU");
                        break;
                    case CAN_ACU_CAN_ID:
                        messageReceivedFromControlUnit("ACU");
                        break;
                    case CAN_SCU_CAN_ID:
                        messageReceivedFromControlUnit("SCU");
                        break;
                }
            }
        }
    }
}

void StartCanTxTask(void *argument){
    char canMsg[50];
    for(;;){
        TxHeader.IDE = CAN_ID_STD; // Using Standard ID
        TxHeader.StdId = CAN_VCU_CAN_ID;   // Transmitter's ID (11-bit wide)
        TxHeader.RTR = CAN_RTR_DATA; // Data frame
        TxHeader.DLC = 6; // Length of data bytes
        TxData[0] = 'H'; // ASCII code for 'H'
        TxData[1] = 'i'; // ASCII code for 'i'
        TxData[2] = ' '; // ASCII code for space
        TxData[3] = 'V'; // ASCII code for 'V'
        TxData[4] = 'C'; // ASCII code for 'C'
        TxData[5] = 'U'; // ASCII code for 'U'

        if (HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
            strncpy(canMsg, "SCU couldn't send the message to the CAN Bus.\r\n", sizeof(canMsg) - 1);
            logMessage(canMsg, true);
        }
        else {
            strncpy(canMsg, "SCU sent  'Hi VCU' to the CAN Bus.\r\n", sizeof(canMsg) - 1);
            logMessage(canMsg, true);
        }
    }
}

/* USER CODE END 1 */
