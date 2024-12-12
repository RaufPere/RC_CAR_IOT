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

#define SPEED_TASK_PRIORITY            (configMAX_PRIORITIES-1)
#define SPEED_TASK_STACK_SIZE          (configMINIMAL_STACK_SIZE * 2)

#define METER_PIN P10_4 // Digital pin of speed sensor
#define HOLES 20 // Number of holes in the gear
#define GEAR_RADIUS 0.035// Radius of gear with holes in meters (3.5cm)
#define TIME_INTERVAL 0.25// Time interval for sample in seconds

void speedometer_task(void *pvParameters);

extern QueueHandle_t speedometerQueueHandle;

#endif /* SOURCE_SPEED_H_ */
