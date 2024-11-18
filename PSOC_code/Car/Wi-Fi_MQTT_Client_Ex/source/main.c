/* Header file includes */




#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "mqtt_task.h"

#include "FreeRTOS.h"
#include "task.h"


volatile int uxTopUsedPriority;
TaskHandle_t  bluetooth_task_handle;
TaskHandle_t  mqtt_task_handle;

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



    /* Create the MQTT Client task. */
       rtos_result = xTaskCreate(mqtt_client_task, "MQTT Client task", MQTT_CLIENT_TASK_STACK_SIZE,
                   NULL, MQTT_CLIENT_TASK_PRIORITY, NULL);
       if( pdPASS != rtos_result)
       {
           printf("Failed to create MQTT task! \n");
           CY_ASSERT(0);
       }

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);
}

/* [] END OF FILE */
