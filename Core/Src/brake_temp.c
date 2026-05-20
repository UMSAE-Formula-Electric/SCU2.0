//********************************************************************
//
//	@file 		brake_temp.c
//	@author 	Evan Mack
//	@created	April 13, 2026
//	@brief		Calculates temperature of brakes using thermocouples
//
//*********************************************************************
#include "brake_temp.h"
#include "spi.h"
#include "math.h"
#include "stdio.h"
#include "usart.h"
#include "rtc.h"
#include "cmsis_os.h"
#include "string.h"
#include "logger.h"
#include "can.h"

//Brake Temp Variables and Macros
#define NUM_BRAKE_TEMP_SENSORS 4
bool validTemps;
double brakeTemps[NUM_BRAKE_TEMP_SENSORS];
#define BRAKETEMP_DELAY_MS 20
#define SPI_TIMEOUT_MS 20

//*********************************************************************
// readThermocouples
//
// PURPOSE: This functions reads from the thermocouple sensor using SPI connection through
//			the thermocouple's chip select
//
// INPUT PARAMTERS:
//			port - The port of the chip select
//			pin	 - The pin of the chip select
//
// RETURN:	temp - The temperature in °C of type double
//*********************************************************************
double readThermocouples(GPIO_TypeDef* port, uint16_t pin){
    uint8_t brakeTempReceiveBuffer[4] = {0};
    uint8_t dummyTransmit[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);//Pull CS low to read
    HAL_StatusTypeDef spiReturn = HAL_SPI_TransmitReceive(&hspi3, dummyTransmit,brakeTempReceiveBuffer, 4, SPI_TIMEOUT_MS);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);

    if (spiReturn != HAL_OK) {
            return NAN;
	}

    uint32_t brakeTemp32Bit = (brakeTempReceiveBuffer[0] << 24) |
    						  (brakeTempReceiveBuffer[1] << 16) |
							  (brakeTempReceiveBuffer[2] << 8)  |
							   brakeTempReceiveBuffer[3];//MAX31855 transmits in big endian

    uint32_t raw = brakeTemp32Bit;

    // Check fault bits for an error
    if (raw & 0x7) {
        return NAN;
    }

    int16_t temp = (raw >> 18) & 0x3FFF;//MAX31855 has 14 bit temperature field so shift bits
	if (temp & 0x2000) temp -= 0x4000; //Checks if number is negative and corrects sign

    return temp*0.25; //Max31855 is 0.25°C per bit
}
//*********************************************************************
// StartReadBrakeTempTask
//
// PURPOSE: Main loop for freeRTOS thread. Reads brake temperature through SPI.
//*********************************************************************
void StartReadBrakeTempTask(void *argument){
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

    static char concatenatedTempMessages[256]; // TODO: make sure we don't concatenate past msg size, look at strncat()
    char* timestamp;
    static char* buffer_pos = concatenatedTempMessages;

	for(;;){
		validTemps = true;
		brakeTemps[0] = readThermocouples(GPIOD,GPIO_PIN_8);//Brake Temp 1 Chip Select PD8
		brakeTemps[1] = readThermocouples(GPIOD,GPIO_PIN_9);//Brake Temp 2 Chip Select PD9
		brakeTemps[2] = readThermocouples(GPIOD,GPIO_PIN_10);//Brake Temp 3 Chip Select PD10
		brakeTemps[3] = readThermocouples(GPIOD,GPIO_PIN_11);//Brake Temp 4 Chip Select PD11

		buffer_pos = concatenatedTempMessages;
		*buffer_pos = '\0';

		for(int i = 0; i < NUM_BRAKE_TEMP_SENSORS;i++){
		   timestamp = get_time();
		   if(isnan(brakeTemps[i])){
			   validTemps = false;
		   }
		   else{
			   int remaining = sizeof(concatenatedTempMessages) - (buffer_pos - concatenatedTempMessages);
			   int written = snprintf(buffer_pos, remaining, "[%s] Thermocouple #%d Temp %.2f°C\r\n", timestamp, i, brakeTemps[i]);
			   if (written >= 0 && written < remaining) {
			       buffer_pos += written;
			   }
		   }

		}

		//====================== CAN Messaging ======================
        if(validTemps){
    		uint8_t brakeTempCanData[8];
            convertDoubleToCAN(brakeTemps,brakeTempCanData);
            uint8_t sendStatus = sendCan(&hcan2,brakeTempCanData,8,BRAKE_TEMP_CAN_ID,CAN_RTR_DATA,0);
            if(sendStatus != 0x0)
            {
                logMessage("Brake Temp CAN send failed\r\n",true);
            }
        }
        //===========================================================

        //---------------------- Debug Logging ----------------------
		HAL_USART_Transmit(&husart2, (uint8_t *) concatenatedTempMessages, buffer_pos-concatenatedTempMessages, 1000);
//		buffer_pos = concatenatedTempMessages;
		//-----------------------------------------------------------

		osDelay(pdMS_TO_TICKS(BRAKETEMP_DELAY_MS));
	}
}

