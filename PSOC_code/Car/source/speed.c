/*
 * speed.c
 *
 *  Created on: 27 Nov 2024
 *      Author: raufp
 */

#include "speed.h"

void speedometer_task(void *pvParameters)
{
	bool meter = false;

	for(;;)
	{
		meter = cyhal_gpio_read(METER_PIN);
		printf("Value meter: %d\n", meter);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


