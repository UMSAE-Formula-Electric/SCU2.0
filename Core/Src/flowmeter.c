//***************************************
//
//	@file 		flowmeter.c
//	@author 	Evan Mack
//	@created	January 2nd, 2026
//	@brief		Calculates flowrate from flowmeters
//
//	@datasheet	https://drive.google.com/file/d/1Gfxb1W2j5wihmmY6867XOjltrr32dhPV/view?usp=share_link
//
//***************************************

#include "flowmeter.h"
#include "usart.h"
#include "rtc.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>
#include "tim.h"
#include "logger.h"
#include "can.h"
#include "FreeRTOS.h"
#include "task.h"

//Flowmeter Variables and Macros
volatile int i = 0;
volatile uint32_t flowmeter_pulse_count = 0;
const int PPL = 2200;													// pulse per liter
const int PERIOD = 1000;													// time in ms

#define UART_TIMEOUT_MS 50

static const double conversionFactor = (1000/PERIOD)*(1.0/1000.0);		// convert L/s to m^3/s

//*********************************************************************
// calculateFlowrate
//
// PURPOSE: This function calculates the flowrate from the number of pulses.
//
// INPUT PARAMTERS:
//			nothing - flowmeter_pulse_count is global
//
// RETURN:	flowrate - flowrate in m^3/s of type double
//*********************************************************************
double calculateFlowrate(){
    volatile double flowrate;

    flowrate = ((double)flowmeter_pulse_count/(double)PPL)*conversionFactor;// calculate flowrate

    return flowrate;
}
//*********************************************************************
// flowmeterTask
//
// PURPOSE: Main loop for processing flowmeter. Gets the count of flowmeter pulses
// 			and calculate the flowrate.
//*********************************************************************
void flowmeterTask(void){
  	flowmeter_pulse_count = __HAL_TIM_GET_COUNTER(&htim12); //PB14
      __HAL_TIM_SetCounter(&htim12, 0);


      // TODO: This inline math results in L/s and ignores the conversionFactor to m^3/s.
	  // Not sure if this is for testing but need to replace this line with: double flowrate = calculateFlowrate();
  	double flowrate = (double)flowmeter_pulse_count/(double)PPL;

  	//====================== CAN Messaging ======================
  	// TODO: Array is 2 bytes, but sendCan transmits 8.
	// Either change array to `uint8_t flowmeterCanData[8] = {0};` or change sendCan length to 2
    uint8_t flowmeterCanData[2];
    convertFlowrateToCAN(flowrate,flowmeterCanData);
    uint8_t sendStatus = sendCan(&hcan2,flowmeterCanData,8,FLOW_METER_CAN_ID,CAN_RTR_DATA,0);
    if(sendStatus != 0x0)
    {
        logMessage("Flowmeter CAN send failed\r\n",true);
    }
    //===========================================================

    //---------------------- Debug Logging ----------------------
  	char ms[50];
  	snprintf(ms, sizeof(ms), "Flowmeter pulse count %d: %lu \r\n",i, flowmeter_pulse_count);
  	HAL_USART_Transmit(&husart2, (uint8_t*)ms, strlen(ms), UART_TIMEOUT_MS);

  	char msg[50];
  	snprintf(msg, sizeof(msg), "Flowrate %d: %.4f \r\n",i, flowrate);
  	HAL_USART_Transmit(&husart2, (uint8_t*)msg, strlen(msg), UART_TIMEOUT_MS);
  	i = i + 1;
  	//-----------------------------------------------------------
}

void StartFlowmeterTask(void *argument) {
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        flowmeterTask();
    }
}
