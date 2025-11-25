//********************************************************************
//
//	@file 		thermistor.c
//	@author 	Evan Mack
//	@created	Nov 25, 2025
//	@brief		Calculates temperature
//
//*********************************************************************
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
const uint32_t constResistance = 2000; //TODO: Change to 10k ohm to match electrical schematic

#define NUM_TEMPERATURE_SENSORS 4 // a define instead of a const int to prevent variably modified at file scope error

double volatile temperatures[NUM_TEMPERATURE_SENSORS];
double volatile naturalLogR;
double volatile temperature;
double volatile R_NTC;

// takes the input voltage and returns the resistance
void get_NTC_Resistance(double voltageReading){
    if (voltageReading >= (V_DD - 0.1) || voltageReading <= 0){ R_NTC = 0;}
    //else {R_NTC = (V_DD / voltageReading - 1) * constResistance;}
    else {R_NTC = (voltageReading / (V_DD - voltageReading)) * constResistance;}
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
	voltages[0] = ADC_TO_Voltage * ADC_get_val(MOTOR_FRONT_THERMISTOR); //Pin A5
	voltages[1] = ADC_TO_Voltage * ADC_get_val(MOTOR_BACK_THERMISTOR);
	voltages[2] = ADC_TO_Voltage * ADC_get_val(MOTOR_CONTROLLER_FRONT_THERMISTOR);
	voltages[3] = ADC_TO_Voltage * ADC_get_val(MOTOR_CONTROLLER_BACK_THERMISTOR);
}
//*********************************************************************
// StartReadShocksTask
//
// PURPOSE: Main loop for freeRTOS thread. Waits for ADC conversion and
//			translates voltages into temperature
//*********************************************************************
void StartReadTempTask(void *argument){
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

    static char concatenatedTempMessages[256]; // TODO: make sure we don't concatenate past msg size, look at strncat()
    char* time;
    static char* buffer_pos = concatenatedTempMessages;
    double voltages[NUM_TEMPERATURE_SENSORS];

    for (;;){
        if (newData_thermistor == 1) {
            int written = 0;
            // Array of voltages passed by reference
            readTemperatureSensorVoltageFromADC(voltages);

            for(int i = 0; i < 1; i++) { //NUM_TEMPERATURE_SENSORS (only testing one)
                temperatures[i] = getTemperature(voltages[i]);
                time = get_time();
//                /* TODO: correlate the index "i" with the correct physical ADC channel
//                 since the index may not align with the correct channel in the future */
                int written = sprintf(buffer_pos, "[%s] ADC %d %.5f \tTemperature: %f\r\n", time, i, voltages[i], temperatures[i]);
                buffer_pos += written;
                vTaskDelay(pdMS_TO_TICKS(250));  // Delay for 1000 ms
            }

            /* TODO SCU#35 */
            /* Logging Starts */
           HAL_USART_Transmit(&husart2, (uint8_t *) concatenatedTempMessages, buffer_pos-concatenatedTempMessages, 1000);
            /* Logging Ends */
           buffer_pos = concatenatedTempMessages;

           newData_thermistor = 0;					// reset ADC conversion flag
           osDelay(pdMS_TO_TICKS(5));
       }

       osThreadYield();
   }
}
