//****************************************************
//
//	@file 		wheel_speed.c
//	@author 	Matthew Mora
//	@created	September 14th, 2023
//	@brief		Calculates wheel speed from wheel speed sensors
//
//****************************************************

#include "wheel_speed.h"
#include "cmsis_os.h"

// define statements
#define WHEEL_DIAMETER		1							// In meters
#define WHEEL_CIRCUMFERENCE (WHEEL_DIAMETER * 3.14159)	// In meters
#define NUM_TEETH			10							// number of teeth on rotary encoder

#define DELAY 500										// time in ms

static const double conversionFactor = (1000/DELAY);		// convert pulses to m/s

extern volatile int wheel_FL_pulse_count;
extern volatile int wheel_FR_pulse_count;
extern volatile int wheel_RL_pulse_count;
extern volatile int wheel_RR_pulse_count;

// gets wheel speeds as m/s
double calculateWheelSpeed(int pulse_count){
	volatile double wheel_speed;

	wheel_speed = pulse_count*conversionFactor*WHEEL_CIRCUMFERENCE/NUM_TEETH;	// calculate wheel speed

	return wheel_speed;
}

extern void StartReadSpeedsTask(void *argument){
    uint8_t isTaskActivated = (int)argument;
    if (isTaskActivated == 0) {
        osThreadTerminate(osThreadGetId());
    }

	volatile double FL_speed;
	volatile double FR_speed;
	volatile double RL_speed;
	volatile double RR_speed;

	for (;;){
		osDelay(pdMS_TO_TICKS(DELAY));	// wait DELAY ms

		// calculates wheel speeds
		FL_speed = calculateWheelSpeed(wheel_FL_pulse_count);
		FR_speed = calculateWheelSpeed(wheel_FR_pulse_count);
		RL_speed = calculateWheelSpeed(wheel_RL_pulse_count);
		RR_speed = calculateWheelSpeed(wheel_RR_pulse_count);

		/* TODO SCU#35 */
		/* Logging Starts */

		/* Logging Ends */

		// reset counts
		wheel_FL_pulse_count = 0;
		wheel_FR_pulse_count = 0;
		wheel_RL_pulse_count = 0;
		wheel_RR_pulse_count = 0;
	}
}
