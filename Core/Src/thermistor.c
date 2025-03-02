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

#define NUM_TEMPERATURE_SENSORS 4 // a define instead of a const int to prevent variably modified at file scope error

double volatile temperatures[NUM_TEMPERATURE_SENSORS];
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

//*********************************************************************
// readTemperatureSensorVoltageFromADC
//
// PURPOSE: This function gets the temperature sensor voltage value from the ADC reading
//
// INPUT PARAMTERS:
//			voltages - array to store the voltage readings from the ADC
//
// RETURN:	nothing - array is passed by reference
//*********************************************************************
void readTemperatureSensorVoltageFromADC(double *voltages){
	// calculate voltages for each ADC channel connected to a temperature sensor
	voltages[0] = ADC_TO_Voltage * ADC_get_val(THERMISTOR_1);
	voltages[1] = ADC_TO_Voltage * ADC_get_val(THERMISTOR_2);
	voltages[2] = ADC_TO_Voltage * ADC_get_val(THERMISTOR_3);
	voltages[3] = ADC_TO_Voltage * ADC_get_val(THERMISTOR_4);
}

void StartReadTempTask(void *argument){
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

    char concatenatedTempMessages[1024]; // TODO: make sure we don't concatenate past msg size, look at strncat()
    char formattedTempMessage[20];
    char* time;
    double voltages[NUM_TEMPERATURE_SENSORS];

    for (;;){
        if (newData_thermistor == 1) {

            // Array of voltages passed by reference
            readTemperatureSensorVoltageFromADC(voltages);

            for(int i = 0; i < NUM_TEMPERATURE_SENSORS; i++) {
                temperatures[i] = getTemperature(voltages[i]);

                time = get_time();
                strcat(concatenatedTempMessages,time);

                /* TODO: correlate the index "i" with the correct physical ADC channel
                 since the index may not align with the correct channel in the future */
                sprintf(formattedTempMessage, "ADC %d %.5f \n", i, voltages[i]);
                strcat(concatenatedTempMessages,formattedTempMessage);
                sprintf(concatenatedTempMessages, "Temperature: %f\r\n", temperatures[i]);
            }

            /* TODO SCU#35 */
            /* Logging Starts */
            HAL_USART_Transmit(&husart2, (uint8_t *) concatenatedTempMessages, strlen(concatenatedTempMessages), 10);
            /* Logging Ends */

            newData_thermistor = 0;					// reset ADC conversion flag
        }

        osThreadYield();
    }
}
