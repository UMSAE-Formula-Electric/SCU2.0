/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.h
  * @brief   This file contains all the function prototypes for
  *          the adc.c file
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
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN Private defines */
extern volatile int newData_thermistor; // flag to determine if the ADC has finished a read
extern volatile int newData_shock_pot;	// flag to determine if the ADC has finished a read
extern const float V_DD;
extern const double ADC_TO_Voltage;

#define INVALID_ADC_READING 65535U

/* TODO: talk with sparkies and change MC back/front labeling to IN/OUT
 * naming scheme following the microcontroller pinout*/
enum ADC_CHANNEL{
  FL_SHOCK_POTENTIOMETER = 0,
  FR_SHOCK_POTENTIOMETER,
  BL_SHOCK_POTENTIOMETER,
  BR_SHOCK_POTENTIOMETER,
  MOTOR_FRONT_THERMISTOR,
  MOTOR_BACK_THERMISTOR,
  MOTOR_CONTROLLER_FRONT_THERMISTOR,
  MOTOR_CONTROLLER_BACK_THERMISTOR,
  NUM_ADC_CHANNELS
};
/* USER CODE END Private defines */

void MX_ADC1_Init(void);

/* USER CODE BEGIN Prototypes */
  uint32_t ADC_get_val(uint8_t item);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */

