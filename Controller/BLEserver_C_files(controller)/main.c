#include "wiced_bt_stack.h"
#include "cybsp.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include <FreeRTOS.h>
#include <task.h>
#include "cycfg_bt_settings.h"
#include "cts_server.h"
#include "cybsp_bt_config.h"

volatile int uxTopUsedPriority;

TaskHandle_t  button_task_handle;

#define ADC_PIN P10_0 //P10_2
#define ADC_PIN_2 P10_1 //P10_4

cyhal_adc_t adcObj;
cyhal_adc_channel_t adc_chan_0_obj;

cyhal_adc_channel_t adc_chan_0_obj1;

TaskHandle_t joystickHandle;

QueueHandle_t JoystickDataQueue;

void JoyStickTask(void * parameters)
{
	JoystickData data;

	for (;;)
	{
		data.x = (cyhal_adc_read(&adc_chan_0_obj) + 2048) * 255 / 4096;

		data.y = (cyhal_adc_read(&adc_chan_0_obj1) + 2048) * 255 / 4096;

		printf("ADC value X: %d\r\n", data.x);
		printf("ADC value Y: %d\r\n", data.y);

		xQueueOverwrite(JoystickDataQueue, &data);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void initADC()
{
	// INIT ADC for joystick
	cy_rslt_t result = cyhal_adc_init(&adcObj, ADC_PIN, NULL);


	if (result != CY_RSLT_SUCCESS)
	{
		printf("Init adc failed\r\n");
	}

	// Initialize ADC channel, allocate channel number 0 to pin ADC_VPLUS1 as this is the first
	// channel initialized
	const cyhal_adc_channel_config_t channel_config =
		{ .enable_averaging = false, .min_acquisition_ns = 220, .enabled = true };
	result = cyhal_adc_channel_init_diff(&adc_chan_0_obj, &adcObj, ADC_PIN, CYHAL_ADC_VNEG,
									   &channel_config);

	// Initialize ADC channel, allocate channel number 0 to pin ADC_VPLUS1 as this is the first
	// channel initialized
	const cyhal_adc_channel_config_t channel_config1 =
		{ .enable_averaging = false, .min_acquisition_ns = 220, .enabled = true };
	result = cyhal_adc_channel_init_diff(&adc_chan_0_obj1, &adcObj, ADC_PIN_2, CYHAL_ADC_VNEG,
									   &channel_config1);

	if (result != CY_RSLT_SUCCESS)
	{
		printf("Init adc channel failed 2\r\n");
	}
}

int main()
{
    cy_rslt_t rslt;
    wiced_result_t result;
    BaseType_t rtos_result;

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the board support package */
    rslt = cybsp_init();

    if (CY_RSLT_SUCCESS != rslt)
    {
        CY_ASSERT(0);
    }

    JoystickDataQueue = xQueueCreate(1,sizeof(JoystickData));

    initADC();

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);

    printf("**********************AnyCloud Example*************************\n");
    printf("**** Current Time Service (CTS) - Server Application Start ****\n");
    printf("***************************************************************\n\n");

    /* Configure platform specific settings for the BT device */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /* Register call back and configuration with stack */
    result = wiced_bt_stack_init(app_bt_management_callback,
                                 &wiced_bt_cfg_settings);

    /* Check if stack initialization was successful */
    if( WICED_BT_SUCCESS == result)
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

    /* Create Button Task for processing button presses */
	rtos_result = xTaskCreate(JoyStickTask, "joystick readout", configMINIMAL_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, &joystickHandle);
	if( pdPASS != rtos_result)
	{
		printf("Failed to create joystick task! \n");
		CY_ASSERT(0);
	}

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* The application should never reach here */
    CY_ASSERT(0) ;
}

/* [] END OF FILE */
