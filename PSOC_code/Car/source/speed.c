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
    bool meter = false;
    bool prevMeter = false;

    int counter = 0;    // Tracks pulses (holes)
    int pulses_per_rotation = HOLES; // Number of holes in the gear

    speedometer_data_t data;

    TickType_t last_wake_time = xTaskGetTickCount(); // Keep track of time
    const TickType_t sample_interval = pdMS_TO_TICKS(TIME_INTERVAL * 1000); // Sampling interval in ticks

    for (;;)
    {
        meter = cyhal_gpio_read(METER_PIN);

        if (meter && !prevMeter)
        {
            // Rising edge detected
            counter++;
        }

        prevMeter = meter;

        // Sample every TIME_INTERVAL seconds
        if (xTaskGetTickCount() - last_wake_time >= sample_interval)
        {
            last_wake_time = xTaskGetTickCount();

            // Calculate RPM
            data.rpm = (counter * 60) / (TIME_INTERVAL * pulses_per_rotation);

            // Calculate speed in m/s
            data.speed_mps = data.rpm * (2 * M_PI * GEAR_RADIUS) / 60;

            // Convert to km/h
            data.speed_kph = data.speed_mps * 3.6;

            xQueueSend(speedometerQueueHandle, &data, 0);

            // Print results
            // printf("Pulses: %d, RPM: %d, Speed: %.2f m/s, %.2f km/h\n", counter, data.rpm, data.speed_mps, data.speed_kph);

            // Reset pulse count for next interval
            counter = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // Small delay to avoid high CPU usage
    }
}




