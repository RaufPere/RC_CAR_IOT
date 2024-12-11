/*
 * speed.c
 *
 *  Created on: 27 Nov 2024
 *      Author: raufp
 */

#include "speed.h"
#include "publisher_task.h"

// Doing this with an interrupt isn't a good idea because it will interrupt all the RTOS tasks as
// hardware interrupts have higher priority. This task isn't more important
void speedometer_task(void *pvParameters)
{
	//vTaskDelay(pdMS_TO_TICKS(500)); // Small delay to avoid high CPU usage
    bool meter = false;
    bool prevMeter = false;

    int counter = 0;    // Tracks pulses (holes)
    int rotations = 0;  // Tracks full rotations
    int pulses_per_rotation = HOLES; // Number of holes in the gear

    speedometer_data_t data;

    TickType_t last_wake_time = xTaskGetTickCount(); // Keep track of time
    const TickType_t sample_interval = pdMS_TO_TICKS(TIME_INTERVAL * 500); // Sampling interval in ticks

    for (;;)
    {
        meter = cyhal_gpio_read(METER_PIN);

        if (meter == true && prevMeter == false)
        {
            // Rising edge detected
            counter++;
        }

        prevMeter = meter;

        if (counter >= pulses_per_rotation)
        {
            // A full rotation has occurred
            counter = 0;
            rotations++;
        }

        // Sample every TIME_INTERVAL seconds
        if (xTaskGetTickCount() - last_wake_time >= sample_interval)
        {
            last_wake_time = xTaskGetTickCount();

            // Calculate RPM
            data.rpm = rotations * 60 / TIME_INTERVAL;

            // Calculate speed in m/s
            data.speed_mps = data.rpm * (2 * M_PI * GEAR_RADIUS) / 60;

            // Convert to km/h
            data.speed_kph = data.speed_mps * 3.6;

            xQueueSend(speedometerQueueHandle, &data, 0);

            // Print results
            // printf("Rotations: %d, RPM: %d, Speed: %.2f m/s, %.2f km/h\n", rotations, data.rpm, data.speed_mps, data.speed_kph);

            // Reset rotations count for next interval
            rotations = 0;
        }

        //printf("Counter: %d\n",counter);
        vTaskDelay(pdMS_TO_TICKS(1)); // Small delay to avoid high CPU usage
    }
}



