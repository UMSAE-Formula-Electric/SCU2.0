//***************************
//
//	@file 		IMU.h
//	@author 	Matthew Mora
//	@created	Augues 6th, 2023
//	@brief		Processes data transmitted by the IMU via CAN
//
//***************************

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "usart.h"
#include "task.h"

// avoid defining multiple times
#ifndef _IMU
#define _IMU

// function prototypes
int queueAccelerationPacket(uint8_t data[]);
int queueAngularRatePacket(uint8_t data[]);

#endif // ifndef _IMU
