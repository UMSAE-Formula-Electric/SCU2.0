//***************************
//
//	@file 		flowmeter.c
//	@author 	Evan Mack
//	@created	April 27, 2023
//	@brief		Calculates flowrate from flowmeters
//
//	@datasheet	https://drive.google.com/file/d/1Gfxb1W2j5wihmmY6867XOjltrr32dhPV/view?usp=share_link
//  NOTE: Timers (TIM6 and TIM3) need to be initialized properly in .ioc and main
//  Only general code is given
//***************************

#include "flowmeter.h"
#include "usart.h"
#include "rtc.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

volatile int i = 0;
volatile uint32_t flowmeter_pulse_count = 0;
const int PPL = 2200;													// pulse per liter


// gets flowrate as m^3/s
double calculateFlowrate(){
    volatile double flowrate;

    flowrate = flowmeter_pulse_count/PPL;// calculate flowrate
    flowmeter_pulse_count = 0;								// reset pulse_count

    return flowrate;
}

//HAL_TIM_Base_Start_IT(&htim6);
//HAL_TIM_Base_Start(&htim3);



// calculates flowrate every DELAY ms
/*
void StartReadFlowTask(void *argument){
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

    volatile double flowrate;
    char tempMsg[50];
    char* time;

    for (;;){
        flowrate = calculateFlowrate();		// calculates flowrate

        /* TODO SCU#35 */
        /* Logging Starts */
/*
        time = get_time();
        HAL_USART_Transmit(&husart2, (uint8_t *) time, strlen(time), 10);

        sprintf(tempMsg, ",%f,,,,\r\n", flowrate);
        HAL_USART_Transmit(&husart2, (uint8_t *) tempMsg, strlen(tempMsg), 10);
        /* Logging Ends */
/*
        osDelay(DELAY);
    }
}
*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {

    	//HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

    	flowmeter_pulse_count = __HAL_TIM_GET_COUNTER(&htim3);
        __HAL_TIM_SetCounter(&htim3, 0);

    	char ms[50];
        sprintf(ms, "Flowmeter pulse count %d: %lu \r\n",i, flowmeter_pulse_count);
        HAL_UART_Transmit(&huart2, (uint8_t*)ms, strlen(ms), HAL_MAX_DELAY);

    	float flowrate = (float)flowmeter_pulse_count/(float)PPL;

        char msg[50];
        sprintf(msg, "Flowrate %d: %.4f \r\n",i, flowrate);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
        i = i + 1;
    }
}
