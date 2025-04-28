//********************************************************************
//
//	@file 		shock_pot.c
//	@author 	Matthew Mora
//	@created	Nov 19, 2022
//	@brief		Calculates flowrate from flowmeters
//
//	@datasheet	https://drive.google.com/file/d/1g9wjH6BT5--y21_IYlu2G4MbX3KbiAo5/view?usp=share_link
//
//*********************************************************************

#include "shock_pot.h"
#include "stdio.h"
#include "adc.h"
#include <usart.h>
#include "rtc.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

// variables defined in shock_pot.c
#define NUM_SHOCK_POTS 4 // a define instead of a const int to prevent variably modified at file scope error

const float MAX_DISTANCE = 50;	// max travel of shock potentiometer in mm
volatile double distance[NUM_SHOCK_POTS];	// holds distances read from each ADC input, each shock pot has its own ADC channel

//*********************************************************************
// getDistanceFromVoltage
//
// PURPOSE: This function translates voltages to distance in millimeters (mm)
//
// INPUT PARAMTERS:
//			voltage - voltage measured across the shock pot
//
// RETURN:	distance in mm of type double
//*********************************************************************
double getDistanceFromVoltage(double voltage){
	double distance = MAX_DISTANCE * voltage / V_DD;
	return distance;
}

//*********************************************************************
// readShockPotsVoltageFromADC
//
// PURPOSE: This function gets the shock pot voltage value from the ADC reading
//
// INPUT PARAMTERS:
//			voltages - array to store the voltage readings from the ADC
//
// RETURN:	nothing - array is passed by reference
//*********************************************************************
void readShockPotsVoltageFromADC(double *voltages){
	// calculate distances for each ADC channel connected to a shock pot
	voltages[0] = ADC_TO_Voltage * ADC_get_val(FL_SHOCK_POTENTIOMETER);
	voltages[1] = ADC_TO_Voltage * ADC_get_val(FR_SHOCK_POTENTIOMETER);
	voltages[2] = ADC_TO_Voltage * ADC_get_val(BL_SHOCK_POTENTIOMETER);
	voltages[3] = ADC_TO_Voltage * ADC_get_val(BR_SHOCK_POTENTIOMETER);
}

//*********************************************************************
// StartReadShocksTask
//
// PURPOSE: Main loop for freeRTOS thread. Waits for ADC conversion and
//			translates voltages into distance
//*********************************************************************
void StartReadShocksTask(void *argument){
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

    static char concatenatedDistanceMessages[256]; // TODO: make sure we don't concatenate past msg size, look at strncat()
    //char formattedDistanceMessage[40];
    char* time;
    static char* buffer_pos;
    double voltages[NUM_SHOCK_POTS];

    for (;;){
        if (newData_shock_pot == 1){

            // Array of voltages passed by reference
            //readShockPotsVoltageFromADC(voltages);

            for(int i = 0; i < NUM_SHOCK_POTS; i++) {
                distance[i] = getDistanceFromVoltage(voltages[i]);

                time = get_time();
//                /* TODO: correlate the index "i" with the correct physical ADC channel
//                 since the index may not align with the correct channel in the future */
                int written = sprintf(buffer_pos, "[%s] ADC %d %.5f \tDistance: %f\r\n", time, i, voltages[i], distance[i]);
                buffer_pos += written;
            }

            /* TODO SCU#35 */
            /* Logging Starts */
            HAL_USART_Transmit(&husart2, (uint8_t *) concatenatedDistanceMessages, buffer_pos-concatenatedDistanceMessages, 1000);
            /* Logging Ends */
            buffer_pos = concatenatedDistanceMessages;

            newData_shock_pot = 0;					// reset ADC conversion flag
            osDelay(pdMS_TO_TICKS(5));
        }

        osThreadYield();
    }
}
