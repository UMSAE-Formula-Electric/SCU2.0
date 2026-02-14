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

// define statements
#define WHEEL_DIAMETER		0.406							// In meters
#define WHEEL_CIRCUMFERENCE (WHEEL_DIAMETER * 3.14159)	// In meters
#define NUM_TEETH			37							// number of teeth on rotary encoder


extern volatile int wheel_FL_pulse_count;
extern volatile int wheel_FR_pulse_count;
extern volatile int wheel_RL_pulse_count;
extern volatile int wheel_RR_pulse_count;

// gets wheel speeds as m/s
double calculateWheelSpeed(int pulse_count){
	volatile double wheel_speed;

	wheel_speed = pulse_count*WHEEL_CIRCUMFERENCE/NUM_TEETH;	// calculate wheel speed

	return wheel_speed;
}

void wheelspeedTask(void) {
	volatile double FL_speed;
	volatile double FR_speed;
	volatile double RL_speed;
	volatile double RR_speed;


	FL_speed = calculateWheelSpeed(wheel_FL_pulse_count);
	FR_speed = calculateWheelSpeed(wheel_FR_pulse_count);
	RL_speed = calculateWheelSpeed(wheel_RL_pulse_count);
	RR_speed = calculateWheelSpeed(wheel_RR_pulse_count);


    char msgspeed[120];
    sprintf(msgspeed,
             "FL=%.2f m/s, FR=%.2f m/s, RL=%.2f m/s, RR=%.2f m/s\r\n",
             FL_speed, FR_speed, RL_speed, RR_speed);
//			 wheel_FL_pulse_count, wheel_FR_pulse_count, wheel_RL_pulse_count, wheel_RR_pulse_count);

    HAL_USART_Transmit(&husart2, (uint8_t*)msgspeed, strlen(msgspeed), HAL_MAX_DELAY);

	wheel_FL_pulse_count = 0;
	wheel_FR_pulse_count = 0;
	wheel_RL_pulse_count = 0;
	wheel_RR_pulse_count = 0;
}
