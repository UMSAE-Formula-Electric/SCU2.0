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

//volatile int pulse_count = 0;											// tracks number of rising edges from flowmeter
const int PPL = 2200;													// pulse per liter
const int DELAY = 500;													// time in ms
static const double conversionFactor = (1000/DELAY)*(1.0/1000.0);		// convert L/s to m^3/s
extern volatile int flowmeter_pulse_count;

// gets flowrate as m^3/s
double calculateFlowrate(){
    volatile double flowrate;

    flowrate = flowmeter_pulse_count*conversionFactor/PPL;	// calculate flowrate
    flowmeter_pulse_count = 0;								// reset pulse_count

    return flowrate;
}

// calculates flowrate every DELAY ms
void StartReadFlowTask(void *argument){
    volatile double flowrate;
    char tempMsg[50];
    char* time;

    for (;;){
        flowrate = calculateFlowrate();		// calculates flowrate

        /* TODO SCU#35 */
        /* Logging Starts */
        time = get_time();
        HAL_USART_Transmit(&husart2, (uint8_t *) time, strlen(time), 10);

        sprintf(tempMsg, ",%f,,,,\r\n", flowrate);
        HAL_USART_Transmit(&husart2, (uint8_t *) tempMsg, strlen(tempMsg), 10);
        /* Logging Ends */

        osDelay(DELAY);
    }
}
