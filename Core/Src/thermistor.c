//********************************************************************
//
//	@file 		thermistor.c
//	@author 	Evan Mack
//	@created	Nov 25, 2025
//	@brief		Calculates temperature of cooling loop thermistors
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
#include "logger.h"
#include "can.h"

// STEINHART & HART Equation Coefficients
const double A = 1.2794639360E-3;
const double B = 2.6408831422E-4;
const double C = 1.3679771000E-7;

// Thermistor Variables and Macros
const uint32_t TEMP_SENSOR_VOLTAGE_DIVIDER_RESISTOR = 10000; //ohms

#define NUM_TEMPERATURE_SENSORS 4
#define THERMISTOR_DELAY_MS 5

double volatile temperatures[NUM_TEMPERATURE_SENSORS];
double volatile temperatureVoltages[NUM_TEMPERATURE_SENSORS];
double volatile naturalLogR;
double volatile temperature;
double volatile R_NTC;

//*********************************************************************
// get_NTC_Resistance
//
// PURPOSE: This function uses the ADC voltage reading to update the NTC Thermistor resistance
//
// INPUT PARAMTERS:
//			voltageReading - The ADC voltage read value
//
// RETURN:	nothing - resistance is passed by reference
//*********************************************************************
void get_NTC_Resistance(double voltageReading){
    if (voltageReading >= (V_DD - 0.1) || voltageReading <= 0){ R_NTC = 0;}

    else {R_NTC = (voltageReading / (V_DD - voltageReading)) * TEMP_SENSOR_VOLTAGE_DIVIDER_RESISTOR;}
}
//*********************************************************************
// getTemperature
//
// PURPOSE: This function calculates and returns the temperature from the voltage reading
//
// INPUT PARAMTERS:
//			voltageReading - The ADC voltage read value
//
// RETURN:	temperature - The temperature in °C of type double
//*********************************************************************
double getTemperature(double voltageReading){
    get_NTC_Resistance(voltageReading);
    if (R_NTC < 1){temperature = -1;}
    else{
        naturalLogR = log(R_NTC);
        temperature = -273.15 + (1 / (A + B * naturalLogR + C * pow(naturalLogR, 3)));// USING STEINHART & HART EQUATIONS
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
void readTemperatureSensorVoltageFromADC(double *temperatureVoltages){
	// calculate voltages for each ADC channel connected to a temperature sensor
	temperatureVoltages[0] = ADC_TO_Voltage * ADC_get_val(MOTOR_FRONT_THERMISTOR); //PC0 NOT USING ON ePBR26
	temperatureVoltages[1] = ADC_TO_Voltage * ADC_get_val(MOTOR_BACK_THERMISTOR); //PC1 NOT USING ON ePBR26
	temperatureVoltages[2] = ADC_TO_Voltage * ADC_get_val(COOLING_PUMP_THERMISTOR);//PC2
	temperatureVoltages[3] = ADC_TO_Voltage * ADC_get_val(COOLING_RADIATOR_THERMISTOR);//PC3
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

    for (;;){
        if (newData_thermistor == 1) {
        	buffer_pos = concatenatedTempMessages;
        	*buffer_pos = '\0';

            // Array of voltages passed by reference
            readTemperatureSensorVoltageFromADC(temperatureVoltages);

            for(int i = 0; i < NUM_TEMPERATURE_SENSORS ; i++) {
                temperatures[i] = getTemperature(temperatureVoltages[i]);
                time = get_time();
//                /* TODO: correlate the index "i" with the correct physical ADC channel
//                 since the index may not align with the correct channel in the future */
                int remaining = sizeof(concatenatedTempMessages) - (buffer_pos - concatenatedTempMessages);
 			    int written = snprintf(buffer_pos, remaining, "[%s] Thermistor %d %.5f \tTemperature: %f\r\n", time, i, temperatureVoltages[i], temperatures[i]);
 			    if (written > 0 && written < remaining) {
 			       buffer_pos += written;
 			    }

            }
            //====================== CAN Messaging ======================
            uint8_t thermistorCanData[8];
            convertDoubleToCAN(temperatures,thermistorCanData);
            uint8_t sendStatus = sendCan(&hcan2,thermistorCanData,8,COOLING_LOOP_THERMISTOR_CAN_ID,CAN_RTR_DATA,0);
            if(sendStatus != 0x0)
            {
                logMessage("Thermistor CAN send failed\r\n",true);
            }
            //===========================================================

            //---------------------- Debug Logging ----------------------
            HAL_USART_Transmit(&husart2, (uint8_t *) concatenatedTempMessages, buffer_pos-concatenatedTempMessages, 1000);
            buffer_pos = concatenatedTempMessages;
            //-----------------------------------------------------------

            newData_thermistor = 0;	// reset ADC conversion flag
            osDelay(pdMS_TO_TICKS(THERMISTOR_DELAY_MS));
       }
       osThreadYield();
   }
}
