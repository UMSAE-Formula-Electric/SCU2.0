/*
 * thermistor.c
 *
 *  Created on: Aug 6, 2022
 *  Updated on: May 21, 2023
 *      Author: tonyz
 */
#include "thermistor.h"
#include "math.h"
#include "stdio.h"
#include "adc.h"
#include "usart.h"
#include "rtc.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "string.h"

// STEINHART & HART Equation Coefficients
const double A = 1.2794639360E-3;
const double B = 2.6408831422E-4;
const double C = 1.3679771000E-7;

// Conversion Variables
const uint32_t constResistance = 1200;

double volatile temperatures[16];
double volatile naturalLogR;
double volatile temperature;
double volatile R_NTC;

// takes the input voltage and returns the resistance
void get_NTC_Resistance(double voltageReading){
    if (voltageReading >= (V_DD - 0.1) || voltageReading <= 0){ R_NTC = 0;}
    else {R_NTC = (V_DD / voltageReading - 1) * constResistance;}
}

// takes the input voltage and returns the temperature
double getTemperature(double voltageReading){		// USING STEINHART & HART EQUATION
    get_NTC_Resistance(voltageReading);
    if (R_NTC < 1){temperature = -1;}
    else{
        naturalLogR = log(R_NTC);
        temperature = -273.15 + (1 / (A + B * naturalLogR + C * pow(naturalLogR, 3)));
    }
    return temperature;
}

void StartReadTempTask(void *argument){
    char tempMsg[50];
    char* time;

    for (;;){
        if (newData_thermistor == 1) {
            for(int i = 0; i < 16; i++) {
                temperatures[i] = getTemperature(ADC_TO_Voltage * ADC_Readings[i]);
            }

            /* TODO SCU#35 */
            /* Logging Starts */
            time = get_time();
            HAL_USART_Transmit(&husart2, (uint8_t *) time, strlen(time), 10);

            sprintf(tempMsg, ",,%f,", temperatures[0]);
            HAL_USART_Transmit(&husart2, (uint8_t *) tempMsg, strlen(tempMsg), 10);

            sprintf(tempMsg, "%f,", temperatures[1]);
            HAL_USART_Transmit(&husart2, (uint8_t *) tempMsg, strlen(tempMsg), 10);

            sprintf(tempMsg, "%f,", temperatures[2]);
            HAL_USART_Transmit(&husart2, (uint8_t *) tempMsg, strlen(tempMsg), 10);

            sprintf(tempMsg, "%f\r\n", temperatures[3]);
            HAL_USART_Transmit(&husart2, (uint8_t *) tempMsg, strlen(tempMsg), 10);
            /* Logging Ends */
        }

        newData_thermistor = 0;
        osThreadYield();
    }
}
