//****************************************************
//
//	@file 		wheel_speed.c
//	@author 	Evan Mack & Cedric Caparas
//	@created	January 13, 2026
//	@brief		Calculates wheel speed from wheel speed sensors
//
//****************************************************

#include "wheel_speed.h"
#include "usart.h"
#include "cmsis_os.h"
#include "logger.h"
#include "can.h"

//Wheel Speed Variables and Macros
#define WHEEL_DIAMETER		0.406						// In meters
#define WHEEL_CIRCUMFERENCE (WHEEL_DIAMETER * 3.14159)	// In meters
#define NUM_TEETH			23							// number of teeth on gear
#define NUM_WHEELSPEEDS		4

extern volatile int wheel_FL_pulse_count;
extern volatile int wheel_FR_pulse_count;
extern volatile int wheel_RL_pulse_count;
extern volatile int wheel_RR_pulse_count;

//*********************************************************************
// calculateWheelSpeed
//
// PURPOSE: This function calculates the wheel speed from the number of pulses.
//
// INPUT PARAMTERS:
//			pulse_count - the pulse count for each wheel speed sensor
//
// RETURN:	wheel_speed - speed of the wheel in m/s of type double
//*********************************************************************
double calculateWheelSpeed(int pulse_count){
	volatile double wheel_speed;

	wheel_speed = pulse_count*WHEEL_CIRCUMFERENCE/NUM_TEETH;	// calculate wheel speed in m/s

	return wheel_speed;
}
//*********************************************************************
// wheelSpeedTask
//
// PURPOSE: Main loop for processing the wheel speeds. Gets the count of wheel speed pulses
// 			and calculate the wheel speed.
//*********************************************************************
void wheelSpeedTask(void) {
	double wheelSpeeds[NUM_WHEELSPEEDS];

	wheelSpeeds[0] = calculateWheelSpeed(wheel_FL_pulse_count);//FL PA8
	wheelSpeeds[1] = calculateWheelSpeed(wheel_FR_pulse_count);//FR PA9
	wheelSpeeds[2] = calculateWheelSpeed(wheel_RL_pulse_count);//RL PA10
	wheelSpeeds[3] = calculateWheelSpeed(wheel_RR_pulse_count);//RR PA11

	//====================== CAN Messaging ======================
    uint8_t wheelSpeedCanData[8];
    convertDoubleToCAN(wheelSpeeds,wheelSpeedCanData);
    uint8_t sendStatus = sendCan(&hcan2,wheelSpeedCanData,8,WHEEL_SPEED_CAN_ID,CAN_RTR_DATA,0);
    if(sendStatus != 0x0)
    {
        logMessage("Wheel Speed CAN send failed\r\n",true);
    }
    //===========================================================

    //---------------------- Debug Logging ----------------------
    char msgspeed[120];
    sprintf(msgspeed,
             "FL=%d m/s, FR=%d m/s, RL=%d m/s, RR=%d m/s\r\n",
			 //wheelSpeeds[0], wheelSpeeds[1], wheelSpeeds[2], wheelSpeeds[3]);
			 wheel_FL_pulse_count, wheel_FR_pulse_count, wheel_RL_pulse_count, wheel_RR_pulse_count);

    HAL_USART_Transmit(&husart2, (uint8_t*)msgspeed, strlen(msgspeed), HAL_MAX_DELAY);
    //-----------------------------------------------------------

    //Reset counts
	wheel_FL_pulse_count = 0;
	wheel_FR_pulse_count = 0;
	wheel_RL_pulse_count = 0;
	wheel_RR_pulse_count = 0;
}
