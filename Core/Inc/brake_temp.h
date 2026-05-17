/*
 * brake_temp.h
 *
 *  Created on: Apr 28, 2026
 *      Author: EvanJ
 */

#ifndef INC_BRAKE_TEMP_H_
#define INC_BRAKE_TEMP_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"

double readThermocouples(GPIO_TypeDef* port, uint16_t pin);
void StartReadBrakeTempTask(void *argument);

#endif /* INC_BRAKE_TEMP_H_ */
