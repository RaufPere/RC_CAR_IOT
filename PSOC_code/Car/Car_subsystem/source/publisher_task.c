/******************************************************************************
* File Name:   publisher_task.c
*
* Description: This file contains the task that sets up the user button GPIO 
*              for the publisher and publishes MQTT messages on the topic
*              'MQTT_PUB_TOPIC' to control a device that is actuated by the
*              subscriber task. The file also contains the ISR that notifies
*              the publisher task about the new device state to be published.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"

/* Task header files */
#include "publisher_task.h"
#include "mqtt_task.h"
#include "subscriber_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"

#include "MPU6050.h"
/******************************************************************************
* Macros
******************************************************************************/
/* Interrupt priority for User Button Input. */
#define USER_TIMER_INTR_PRIORITY          (3)
#define TIMER_INT_PRIORITY       (3u)
#define TIMER_TARGET_FREQUENCY   (10000u)
#define TIMER_COUNT_PERIOD       (9999u)
#define MQTT_PAYLOAD_SIZE (145u)
/* Interrupt priority for User Button Input. */
#define USER_BTN_INTR_PRIORITY          (3)

/* The maximum number of times each PUBLISH in this example will be retried. */
#define PUBLISH_RETRY_LIMIT             (10)

/* A PUBLISH message is retried if no response is received within this 
 * time (in milliseconds).
 */
#define PUBLISH_RETRY_MS                (1000)

/* Queue length of a message queue that is used to communicate with the 
 * publisher task.
 */
#define PUBLISHER_TASK_QUEUE_LENGTH     (3u)

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void publisher_init(void);
static void publisher_deinit(void);

void print_heap_usage(char *msg);

/******************************************************************************
* Global Variables
*******************************************************************************/
/* FreeRTOS task handle for this task. */
TaskHandle_t publisher_task_handle;

/* Handle of the queue holding the commands for the publisher task */
QueueHandle_t publisher_task_q;

/* Structure to store publish message information. */
cy_mqtt_publish_info_t publish_info =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = MQTT_PUB_TOPIC,
    .topic_len = (sizeof(MQTT_PUB_TOPIC) - 1),
    .retain = false,
    .dup = false
};

/*timer object used*/
cyhal_timer_t timer_obj;
/*timer configuration*/
const cyhal_timer_cfg_t timer_cfg =
{
    .compare_value = 0,                  // Timer compare value, not used
    .period        = TIMER_COUNT_PERIOD, // Defines the timer period
    .direction     = CYHAL_TIMER_DIR_UP, // Timer counts up
    .is_compare    = false,              // Don't use compare mode
    .is_continuous = true,               // Run the timer indefinitely
    .value         = 0                   // Initial value of counter
};

/* Structure that stores the callback data for the GPIO interrupt event. */

static void isr_timer(void* callback_arg, cyhal_timer_event_t event);
/******************************************************************************
 * Function Name: publisher_task
 ******************************************************************************
 * Summary:
 *  Task that sets up the user button GPIO for the publisher and publishes 
 *  MQTT messages to the broker. The user button init and deinit operations,
 *  and the MQTT publish operation is performed based on commands sent by other
 *  tasks and callbacks over a message queue.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void publisher_task(void *pvParameters)
{
    /* Status variable */
    cy_rslt_t result;

    publisher_data_t publisher_q_data;

    /* Command to the MQTT client task */
    mqtt_task_cmd_t mqtt_task_cmd;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Initialize and set-up the user button GPIO. */
    publisher_init();

    /* Create a message queue to communicate with other tasks and callbacks. */
    publisher_task_q = xQueueCreate(PUBLISHER_TASK_QUEUE_LENGTH, sizeof(publisher_data_t));

    while (true)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(publisher_task_q, &publisher_q_data, portMAX_DELAY))
        {
            switch(publisher_q_data.cmd)
            {
                case PUBLISHER_INIT:
                {
                    /* Initialize and set-up the user button GPIO. */
                    publisher_init();
                    break;
                }

                case PUBLISHER_DEINIT:
                {
                    /* Deinit the user button GPIO and corresponding interrupt. */
                    publisher_deinit();
                    break;
                }

                case PUBLISH_MQTT_MSG:
                {
                	// Wake the MPU6050 up from sleep mode
                	                	MPU6050_i2c_config();
                	                	// Read accelerometer registers
                	                	MPU6050_i2c_accelerometer(&publisher_q_data.accel_x, &publisher_q_data.accel_y,
                	                							&publisher_q_data.accel_z, &publisher_q_data.magnitude);
                	                	// Read temperature registers
                	                	MPU6050_i2c_tempRead(&publisher_q_data.temperature);
                	                	// Read gyroscope registers
                	                	MPU6050_i2c_gyroscoop(&publisher_q_data.gyro_x, &publisher_q_data.gyro_y, &publisher_q_data.gyro_z);
                	                    /* Publish the data received over the message queue. */
                	                	char payload_buffer[MQTT_PAYLOAD_SIZE];  // Allocate enough space for the formatted string

                	                	// Format the data into the buffer as JSON or similar format
                	                	snprintf(payload_buffer, sizeof(payload_buffer),
                	                	         "{"
                	                	         "\"accel_x\": %.2f, \"accel_y\": %.2f, \"accel_z\": %.2f, \"magnitude\": %.2f, "
                	                	         "\"gyro_x\": %.2f, \"gyro_y\": %.2f, \"gyro_z\": %.2f, "
                	                	         "\"temperature\": %.2f"
                	                	         "}",
                	                	         publisher_q_data.accel_x, publisher_q_data.accel_y, publisher_q_data.accel_z,
                	                	         publisher_q_data.magnitude,
                	                	         publisher_q_data.gyro_x, publisher_q_data.gyro_y, publisher_q_data.gyro_z,
                	                	         publisher_q_data.temperature);
                	                	publish_info.payload = payload_buffer;
                	                    publish_info.payload_len = strlen(publish_info.payload);

                    //printf("\nPublisher: Publishing '%s' on the topic '%s'\n",
                      //     (char *) publish_info.payload, publish_info.topic);

                    result = cy_mqtt_publish(mqtt_connection, &publish_info);

                    if (result != CY_RSLT_SUCCESS)
                    {
                        printf("  Publisher: MQTT Publish failed with error 0x%0X.\n\n", (int)result);

                        /* Communicate the publish failure with the the MQTT 
                         * client task.
                         */
                        mqtt_task_cmd = HANDLE_MQTT_PUBLISH_FAILURE;
                        xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
                    }

                    print_heap_usage("publisher_task: After publishing an MQTT message");
                    break;
                }
            }
        }
    }
}

/******************************************************************************
 * Function Name: publisher_init
 ******************************************************************************
 * Summary:
 *  Function that initializes and sets-up the user button GPIO pin along with  
 *  its interrupt.
 * 
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void publisher_init(void)
{

    cy_rslt_t result;
    I2C_PDL_Setup();
    	/* Initialize the timer object. Does not use pin output ('pin' is NC) and
    	     * does not use a pre-configured clock source ('clk' is NULL) */
    	    result = cyhal_timer_init(&timer_obj, NC, NULL);

    	    /*Apply timer configuration such as period, count direction, run mode, etc*/
    	    if (CY_RSLT_SUCCESS == result)
    	    {
    	        result = cyhal_timer_configure(&timer_obj, &timer_cfg);
    	    }

    	    /* Set the frequency of timer to 10000 Hz */
    	    if (CY_RSLT_SUCCESS == result)
    	    {
    	        result = cyhal_timer_set_frequency(&timer_obj, TIMER_TARGET_FREQUENCY);
    	    }

    	    /* register timer interrupt and enable */
    	    if (CY_RSLT_SUCCESS == result)
    	    {
    	        /* Assign the ISR to execute on timer interrupt */
    	        cyhal_timer_register_callback(&timer_obj, isr_timer, NULL);

    	        /* Set the event on which timer interrupt occurs and enable it */
    	        cyhal_timer_enable_event(&timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT,
    	                TIMER_INT_PRIORITY, true);

    	        /* Start the timer with the configured settings */
    	        result = cyhal_timer_start(&timer_obj);
    	    printf("\nTimer (1s) to periodically publish on the topic '%s'...\n",
               publish_info.topic);
    	    }
    	    else printf("Timer setup failed... publisher_task.c\n");
}

/******************************************************************************
 * Function Name: publisher_deinit
 ******************************************************************************
 * Summary:
 *  Cleanup function for the publisher task that disables the user button
 *  interrupt and deinits the user button GPIO pin.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void publisher_deinit(void)
{
cyhal_timer_stop(&timer_obj);
}

/******************************************************************************
 * Function Name: isr_button_press
 ******************************************************************************
 * Summary:
 *  GPIO interrupt service routine. This function detects button
 *  presses and sends the publish command along with the data to be published
 *  to the publisher task over a message queue. Based on the current device
 *  state, the publish data is set so that the device state gets toggled.
 *
 * Parameters:
 *  void *callback_arg : pointer to variable passed to the ISR (unused)
 *  cyhal_gpio_event_t event : GPIO event type (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/

static void isr_timer(void* callback_arg, cyhal_timer_event_t event)
{
	//cyhal_timer_stop(&timer_obj);

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	// This function will be called every time the timer overflows (1000 ms in this case)
	publisher_data_t publisher_q_data;
    (void)callback_arg;
    (void)event;
    /* Assign the publish command to be sent to the publisher task. */
    publisher_q_data.cmd = PUBLISH_MQTT_MSG;
    /* Assign the publish message payload so that the device state toggles. */
    /* Send the command and data to publisher task over the queue */
    xQueueSendFromISR(publisher_task_q, &publisher_q_data, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* [] END OF FILE */
