//***************************
//
//	@file 		flowmeter.c
//	@author 	Matthew Mora
//	@created	January 2nd, 2023
//	@brief		Calculates flowrate from flowmeters
//
//	@datasheet	https://drive.google.com/file/d/1Gfxb1W2j5wihmmY6867XOjltrr32dhPV/view?usp=share_link
//
//***************************

#include "flowmeter.h"
#include "usart.h"
#include "rtc.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>
#include "tim.h"

volatile int i = 0;
volatile uint32_t flowmeter_pulse_count = 0;
const int PPL = 2200;													// pulse per liter
const int DELAY = 500;													// time in ms
static const double conversionFactor = (1000/DELAY)*(1.0/1000.0);		// convert L/s to m^3/s

double calculateFlowrate(){
    volatile double flowrate;

    flowrate = (double)flowmeter_pulse_count/(double)PPL;//*conversionFactor/PPL;	// calculate flowrate


    return flowrate;
}
// calculates flowrate every DELAY ms
void flowmeterTask(void){
  	//HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

  	flowmeter_pulse_count = __HAL_TIM_GET_COUNTER(&htim12);
      __HAL_TIM_SetCounter(&htim12, 0);

  	char ms[50];
      sprintf(ms, "Flowmeter pulse count %d: %lu \r\n",i, flowmeter_pulse_count);
      HAL_USART_Transmit(&husart2, (uint8_t*)ms, strlen(ms), HAL_MAX_DELAY);

  	float flowrate = (float)flowmeter_pulse_count/(float)PPL;

      char msg[50];
      sprintf(msg, "Flowrate %d: %.4f \r\n",i, flowrate);
      HAL_USART_Transmit(&husart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
      i = i + 1;
}
