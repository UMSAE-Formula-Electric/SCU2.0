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



// calculates flowrate every DELAY ms
void StartReadFlowTask(void *argument){
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }


    for (;;){


        osDelay(5);
    }
}
