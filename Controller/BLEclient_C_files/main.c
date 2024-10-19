#include "cybsp_bt_config.h"
#include "wiced_bt_stack.h"
#include "cycfg_bt_settings.h"
#include "cybsp.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include <FreeRTOS.h>
#include <task.h>
#include "cts_client.h"

/*******************************************************************************
*        Variable Definitions
*******************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

/* FreeRTOS task handle for button task. Button task is used to start advertisment
 * or enable/disable notification from peer */
TaskHandle_t  button_task_handle;

/******************************************************************************
 *                          Function Definitions
 ******************************************************************************/
/*
 *  Entry point to the application. Set device configuration and start Bluetooth
 *  stack initialization.  The actual application initialization will happen
 *  when stack reports that Bluetooth device is ready.
 */

int main()
{
    cy_rslt_t cy_result;
    wiced_result_t wiced_result;
    BaseType_t rtos_result;

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the board support package */
    cy_result = cybsp_init();

    if (CY_RSLT_SUCCESS != cy_result)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);

    printf("**********************AnyCloud Example*************************\n");
    printf("**** Current Time Service (CTS) - Client Application Start ****\n");
    printf("***************************************************************\n\n");

    /* Configure platform specific settings for the BT device */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /* Register call back and configuration with stack */
    wiced_result = wiced_bt_stack_init(app_bt_management_callback,
                                       &wiced_bt_cfg_settings);

    /* Check if stack initialization was successful */
    if( WICED_BT_SUCCESS == wiced_result)
    {
        printf("Bluetooth Stack Initialization Successful \n");
    }
    else
    {
        printf("Bluetooth Stack Initialization failed!! \n");
        CY_ASSERT(0);
    }

    /* Create Button Task for processing button presses */
     rtos_result = xTaskCreate(button_task,"button_task", BUTTON_TASK_STACK_SIZE,
                               NULL, BUTTON_TASK_PRIORITY, &button_task_handle);
    if( pdPASS != rtos_result)
    {
        printf("Failed to create Button task! \n");
        CY_ASSERT(0);
    }

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* The application should never reach here */
    CY_ASSERT(0) ;
}

/* [] END OF FILE */
