/*
 * sd_card_benchmark.c
 *
 *  Created on: Apr 28, 2024
 *      Author: niko
 *
 *   This is a dummy task that i'm using the benchmark the sd card's speed
 */
#include "sd_card_benchmark.h"
#include "sd_card.h"
#include "rtc.h"

int writes_left = 333;
#define WRITE_STRING "x"
#define WRITE_STRING_LENGTH 1

void SD_Benchmark_Init(int writes) {
	writes_left = writes;
}

void StartSDCardBenchmark(void) {

	SDCardSync(); // Sync from start so we start with clean slate

	SDCardWrite(get_time(), 20); // start time
	SDCardWrite("\n", 1);

	for(;;) {

		if(writes_left <= 0) {
			SDCardSync(); // for all remaining writes to be forced

			SDCardWrite("\n", 1);
			SDCardWrite(get_time(), 20); // end time
			SDCardSync();

			osThreadTerminate(osThreadGetId());
		}

		if(SDCardWrite(WRITE_STRING, WRITE_STRING_LENGTH)) {
			writes_left--; // decrement write if the write was sucessful
		}

		//osDelay(pdMS_TO_TICKS(500));
	}
}
