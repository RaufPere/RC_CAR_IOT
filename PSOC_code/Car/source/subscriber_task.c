/******************************************************************************
* File Name:   subscriber_task.c
*
* Description: This file contains the task that initializes the user LED GPIO,
*              subscribes to the topic 'MQTT_SUB_TOPIC', and actuates the user LED
*              based on the notifications received from the MQTT subscriber
*              callback.
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
#include "string.h"
#include "FreeRTOS.h"

/* Task header files */
#include "subscriber_task.h"
#include "mqtt_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"
#include "cy_pdl.h"  // Include the Peripheral Driver Library
#include "cyhal.h"   // Include the HAL library
#include "cybsp.h"   // Include the Board Support Package
/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"
#include "MPU6050.h"
#include "motor.h"
/******************************************************************************
* Macros
******************************************************************************/
/* Maximum number of retries for MQTT subscribe operation */
#define MAX_SUBSCRIBE_RETRIES                   (3u)

/* Time interval in milliseconds between MQTT subscribe retries. */
#define MQTT_SUBSCRIBE_RETRY_INTERVAL_MS        (1000)

/* The number of MQTT topics to be subscribed to. */
#define SUBSCRIPTION_COUNT                      (3)

/* Queue length of a message queue that is used to communicate with the 
 * subscriber task.
 */
#define SUBSCRIBER_TASK_QUEUE_LENGTH            (1u)
void EnterULPMode(void);
/******************************************************************************
* Global Variables
*******************************************************************************/
/* Task handle for this task. */
TaskHandle_t subscriber_task_handle;

/* Handle of the queue holding the commands for the subscriber task */
QueueHandle_t subscriber_task_q;

/* Configure the subscription information structure. */
const cy_mqtt_subscribe_info_t subscribe_info_reset_gyro =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = MQTT_SUB_TOPIC_RESET_GYRO,
    .topic_len = (sizeof(MQTT_SUB_TOPIC_RESET_GYRO) - 1)
};

/* Configure the subscription information structure. */
const cy_mqtt_subscribe_info_t subscribe_info_motor =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = MQTT_SUB_TOPIC_MOTOR,
    .topic_len = (sizeof(MQTT_SUB_TOPIC_MOTOR) - 1)
};

/* Configure the subscription information structure. */
const cy_mqtt_subscribe_info_t subscribe_info_LP =
{
    .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = MQTT_SUB_TOPIC_LP,
    .topic_len = (sizeof(MQTT_SUB_TOPIC_LP) - 1)
};

cy_mqtt_subscribe_info_t topicsArray[SUBSCRIPTION_COUNT] = {subscribe_info_motor, subscribe_info_reset_gyro, subscribe_info_LP};

bool LPstate = false;

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void subscribe_to_topic(void);
static void unsubscribe_from_topic(void);
/******************************************************************************
 * Function Name: subscriber_task
 ******************************************************************************
 * Summary:
 *  Task that sets up the user LED GPIO, subscribes to the specified MQTT topic,
 *  and controls the user LED based on the received commands over the message 
 *  queue. The task can also unsubscribe from the topic based on the commands
 *  via the message queue.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void subscriber_task(void *pvParameters)
{
    subscriber_data_t subscriber_q_data;

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Initialize the User LED. */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP,
                    CYBSP_LED_STATE_OFF);

    /* Subscribe to the specified MQTT topic. */
    subscribe_to_topic();

    /* Create a message queue to communicate with other tasks and callbacks. */
    subscriber_task_q = xQueueCreate(SUBSCRIBER_TASK_QUEUE_LENGTH, sizeof(subscriber_data_t));

    while (true)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(subscriber_task_q, &subscriber_q_data, portMAX_DELAY))
        {
            switch(subscriber_q_data.cmd)
            {
                case SUBSCRIBE_TO_TOPIC:
                {
                    subscribe_to_topic();
                    break;
                }

                case UNSUBSCRIBE_FROM_TOPIC:
                {
                    unsubscribe_from_topic();
                    break;
                }

                case UPDATE_DEVICE_STATE:
                {

                    break;
                }
            }
        }
    }
}
/******************************************************************************
 * Function Name: subscribe_to_topic
 ******************************************************************************
 * Summary:
 *  Function that subscribes to the MQTT topic specified by the macro 
 *  'MQTT_SUB_TOPIC'. This operation is retried a maximum of 
 *  'MAX_SUBSCRIBE_RETRIES' times with interval of 
 *  'MQTT_SUBSCRIBE_RETRY_INTERVAL_MS' milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void subscribe_to_topic(void)
{
    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Command to the MQTT client task */
    mqtt_task_cmd_t mqtt_task_cmd;

    /* Subscribe with the configured parameters. */
    for (uint32_t retry_count = 0; retry_count < MAX_SUBSCRIBE_RETRIES; retry_count++)
    {
        result = cy_mqtt_subscribe(mqtt_connection, topicsArray, SUBSCRIPTION_COUNT);
        if (result == CY_RSLT_SUCCESS)
        {
            printf("\nMQTT client subscribed to the topics\n");
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_SUBSCRIBE_RETRY_INTERVAL_MS));
    }

    if (result != CY_RSLT_SUCCESS)
    {
        printf("\nMQTT Subscribe failed with error 0x%0X after %d retries...\n\n", 
               (int)result, MAX_SUBSCRIBE_RETRIES);

        /* Notify the MQTT client task about the subscription failure */
        mqtt_task_cmd = HANDLE_MQTT_SUBSCRIBE_FAILURE;
        xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
    }
}

/******************************************************************************
 * Function Name: mqtt_subscription_callback
 ******************************************************************************
 * Summary:
 *  Callback to handle incoming MQTT messages. This callback prints the 
 *  contents of the incoming message and informs the subscriber task, via a 
 *  message queue, to turn on / turn off the device based on the received 
 *  message.
 *
 * Parameters:
 *  cy_mqtt_publish_info_t *received_msg_info : Information structure of the 
 *                                              received MQTT message
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void mqtt_subscription_callback(cy_mqtt_publish_info_t *received_msg_info)
{
    /* Received MQTT message */
    const char *received_msg = received_msg_info->payload;
    int received_msg_len = received_msg_info->payload_len;
    (void) received_msg_len;
    (void) received_msg;
    /* Data to be sent to the subscriber task queue. */
    subscriber_data_t subscriber_q_data;

    printf("  \nSubsciber: Incoming MQTT message received:\n"
           "    Publish topic name: %.*s\n"
           "    Publish QoS: %d\n"
           "    Publish payload: %.*s\n",
           received_msg_info->topic_len, received_msg_info->topic,
           (int) received_msg_info->qos,
           (int) received_msg_info->payload_len, (const char *)received_msg_info->payload);

    if (received_msg_info->topic[0] == 'M')
    {
    	if(strstr(received_msg_info->payload, "false") != NULL)
		{
    		printf("Motors disabled\n");
			enableMotors = 0;
		}
    	else
    	{
    		printf("Motors enabled\n");
    		enableMotors = 1;
    	}
    }
    if (received_msg_info->topic[0] == 'G')
    {
    	if(!strstr(received_msg_info->payload, "false") != NULL)
    	{
    		printf("Orientation reset!\n");
    		gyro_x_abs = 0.0;
    		gyro_y_abs = 0.0;
    		gyro_z_abs = 0.0;
    	}
    }

    if (received_msg_info->topic[0] == 'L')
	{
		if(!strstr(received_msg_info->payload, "false") != NULL)
		{
			printf("ULP request received!\n");
			LPstate = true;

			// Put the CPU to sleep while the LP Timer runs
			/*
			if (Cy_SysPm_SystemEnterUlp() == CY_SYSPM_SUCCESS)
			{
				printf("Entered default LP mode!\n");
			}
			else
			{
				printf("Failed to enter default LP mode!\n");
			}*/
		}
		else
		{
			printf("ULP disable request received!\n");
			LPstate = false;
/*
			if (Cy_SysPm_SystemEnterLp() == CY_SYSPM_SUCCESS)
			{
				printf("Entered default LP mode!\n");
			}
			else
			{
				printf("Failed to enter default LP mode!\n");
			}*/
		}
	}

    /* Assign the command to be sent to the subscriber task. */
    subscriber_q_data.cmd = UPDATE_DEVICE_STATE;

    /* Send the command and data to subscriber task queue */
    xQueueSend(subscriber_task_q, &subscriber_q_data, portMAX_DELAY);
}

/******************************************************************************
 * Function Name: unsubscribe_from_topic
 ******************************************************************************
 * Summary:
 *  Function that unsubscribes from the topic specified by the macro 
 *  'MQTT_SUB_TOPIC'.
 *
 * Parameters:
 *  void 
 *
 * Return:
 *  void 
 *
 ******************************************************************************/
static void unsubscribe_from_topic(void)
{
    cy_rslt_t result = cy_mqtt_unsubscribe(mqtt_connection, 
                                           (cy_mqtt_unsubscribe_info_t *) &subscribe_info_reset_gyro, 
                                           SUBSCRIPTION_COUNT);

    if (result != CY_RSLT_SUCCESS)
    {
        printf("MQTT Unsubscribe operation failed with error 0x%0X!\n", (int)result);
    }
}

void EnterULPMode(void)
{
    // 1. Reduce the HF clock frequency (HfClk0) for ULP mode
    uint32_t targetHfClkFreqMHz = 25; // Target frequency in MHz (example: 25 MHz)

    // 2. Update the flash wait states for the reduced clock frequency
    Cy_SysLib_SetWaitStates(true, targetHfClkFreqMHz);
}
/* [] END OF FILE */
