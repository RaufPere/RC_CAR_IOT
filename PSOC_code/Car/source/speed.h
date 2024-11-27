/*
 * speed.h
 *
 *  Created on: 27 Nov 2024
 *      Author: raufp
 */

#ifndef SOURCE_SPEED_H_
#define SOURCE_SPEED_H_

#include "cyhal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "stdio.h"
#include "cybsp.h"
#include "cyhal.h"

#define METER_PIN P10_4 // Digital pin of speed sensor

void speedometer_task(void *pvParameters);

#endif /* SOURCE_SPEED_H_ */
