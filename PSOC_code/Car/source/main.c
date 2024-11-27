/* Header file includes */
#include "cybsp_bt_config.h"
#include "wiced_bt_stack.h"
#include "cycfg_bt_settings.h"
#include "cts_client.h"

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "mqtt_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "motor.h"
#include "speed.h"

volatile int uxTopUsedPriority;

TaskHandle_t  bluetooth_task_handle;
TaskHandle_t  mqtt_task_handle;
TaskHandle_t  motor_driver_task_handle;
TaskHandle_t  speedometer_task_handle;

QueueHandle_t joystickDataQueueHandle;

int main()
{
    //wiced_result_t wiced_result;
    BaseType_t rtos_result;

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    cy_rslt_t result;
	
    /* Initialize the board support package. */
    
    result = cybsp_init();
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* To avoid compiler warnings. */
    (void) result;

    /* Enable global interrupts. */
    __enable_irq();
	
    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);
	
    printf("\n\n\nRAUF DARREN IOT RC CAR\n\n\n");

    cy_rslt_t rslt = cyhal_gpio_init(LED_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    rslt = cyhal_gpio_init(METER_PIN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);

    if (rslt != 0)
    {
    	printf("Failed initializing speedometer pin!\n");
    }

    // Queue for sending joystick data from bluetooth task to motordriver task.
    joystickDataQueueHandle = xQueueCreate(1,sizeof(joystickData_t));

    // Create the MQTT Client task.
    rtos_result = xTaskCreate(mqtt_client_task, "MQTT Client task", MQTT_CLIENT_TASK_STACK_SIZE,
	 		   NULL, MQTT_CLIENT_TASK_PRIORITY, NULL);
    if( pdPASS != rtos_result)
    {
	    printf("Failed to create MQTT task! \n");
	    CY_ASSERT(0);
    }

    // Create BLE button Task for processing button presses
    rtos_result = xTaskCreate(bluetooth_task,"bluetooth_task", BLUETOOTH_TASK_STACK_SIZE,
                               NULL, BLUETOOTH_TASK_PRIORITY, &bluetooth_task_handle);
    if( pdPASS != rtos_result)
    {
        printf("Failed to create bluetooth task! \n");
        CY_ASSERT(0);
    }

    // Create motor driver task
    rtos_result = xTaskCreate(motor_driver_task,"motor_driver_task", configMINIMAL_STACK_SIZE*2,
                               NULL, BLUETOOTH_TASK_PRIORITY, &motor_driver_task_handle);
    if( pdPASS != rtos_result)
    {
        printf("Failed to create motor task! \n");
        CY_ASSERT(0);
    }

    // Create speedometer task
	rtos_result = xTaskCreate(speedometer_task,"speedometer_task", configMINIMAL_STACK_SIZE*2,
							   NULL, BLUETOOTH_TASK_PRIORITY, &speedometer_task_handle);
	if( pdPASS != rtos_result)
	{
		printf("Failed to create speedometer task! \n");
		CY_ASSERT(0);
	}

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);
}

/* [] END OF FILE */
