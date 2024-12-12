#include "wiced_bt_stack.h"
#include "cybsp.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include <FreeRTOS.h>
#include <task.h>
#include "cycfg_bt_settings.h"
#include "cts_server.h"
#include "cybsp_bt_config.h"
#include "MAX7219.h"


volatile int uxTopUsedPriority;

TaskHandle_t  button_task_handle;

// ADC pin defines
#define ADC_PIN P10_0 //P10_2
#define ADC_PIN_2 P10_1 //P10_4
#define SEVENSEGM_TASK_PRIORITY (configMAX_PRIORITIES - 2)
// 7 segment display connections
#define S_A P9_0
#define S_B P9_1
#define S_C P9_2
#define S_D P9_3
#define S_E P9_4
#define S_F P9_5
#define S_G P9_6

#define S2_A P5_2
#define S2_B P5_3
#define S2_C P5_4
#define S2_D P5_5
#define S2_E P5_6
#define S2_F P13_0
#define S2_G P13_1

// ADC objects
cyhal_adc_t adcObj;
cyhal_adc_channel_t adc_chan_0_obj;
cyhal_adc_channel_t adc_chan_0_obj1;

// Task and queue handles
TaskHandle_t joystickHandle;
TaskHandle_t sevenSegmentHandle;
TaskHandle_t max7219Handle;
QueueHandle_t JoystickDataQueue;

void DriveSevenSegments(void * parameters)
{
	// Array for segments (A-G) for each digit (0-9)
	const uint8_t digits[10] = {
		0b00111111, // 0
		0b00000110, // 1
		0b01011011, // 2
		0b01001111, // 3
		0b01100110, // 4
		0b01101101, // 5
		0b01111101, // 6
		0b00000111, // 7
		0b01111111, // 8
		0b01101111  // 9
	};

	int speed = 0;
	for (;;)
	{
		// ***Receive speed from a queue***

		if (speed > 99)
		{
			speed = 99;
		}

		int tens = speed / 10;
		int units = speed % 10;

		// Set segments for the tens digit on display 1
		cyhal_gpio_write(S_A, (digits[tens] & 0b00000001) ? 1 : 0);
		cyhal_gpio_write(S_B, (digits[tens] & 0b00000010) ? 1 : 0);
		cyhal_gpio_write(S_C, (digits[tens] & 0b00000100) ? 1 : 0);
		cyhal_gpio_write(S_D, (digits[tens] & 0b00001000) ? 1 : 0);
		cyhal_gpio_write(S_E, (digits[tens] & 0b00010000) ? 1 : 0);
		cyhal_gpio_write(S_F, (digits[tens] & 0b00100000) ? 1 : 0);
		cyhal_gpio_write(S_G, (digits[tens] & 0b01000000) ? 1 : 0);

		// Set segments for the units digit on display 2
		cyhal_gpio_write(S2_A, (digits[units] & 0b00000001) ? 1 : 0);
		cyhal_gpio_write(S2_B, (digits[units] & 0b00000010) ? 1 : 0);
		cyhal_gpio_write(S2_C, (digits[units] & 0b00000100) ? 1 : 0);
		cyhal_gpio_write(S2_D, (digits[units] & 0b00001000) ? 1 : 0);
		cyhal_gpio_write(S2_E, (digits[units] & 0b00010000) ? 1 : 0);
		cyhal_gpio_write(S2_F, (digits[units] & 0b00100000) ? 1 : 0);
		cyhal_gpio_write(S2_G, (digits[units] & 0b01000000) ? 1 : 0);
	}
}


void JoyStickTask(void * parameters)
{
	JoystickData data;

	for (;;)
	{
		data.x = (cyhal_adc_read(&adc_chan_0_obj) + 2048) * 255 / 4096;

		data.y = (cyhal_adc_read(&adc_chan_0_obj1) + 2048) * 255 / 4096;

		//printf("ADC value X: %d\r\n", data.x);
		//printf("ADC value Y: %d\r\n", data.y);

		xQueueOverwrite(JoystickDataQueue, &data);

		vTaskDelay(pdMS_TO_TICKS(10));
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

void initGPIOs()
{
	cy_rslt_t result;

	result = cyhal_gpio_init(S_A, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S_B, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S_C, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S_D, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S_E, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S_F, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S_G, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

	result = cyhal_gpio_init(S2_A, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S2_B, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S2_C, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S2_D, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S2_E, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S2_F, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
	result = cyhal_gpio_init(S2_G, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
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
    initGPIOs();

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);
/*
    printf("**********************AnyCloud Example*************************\n");
    printf("**** Current Time Service (CTS) - Server Application Start ****\n");
    printf("***************************************************************\n\n");
*/
    /* Configure platform specific settings for the BT device */
    //cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /* Register call back and configuration with stack
    result = wiced_bt_stack_init(app_bt_management_callback,
                               &wiced_bt_cfg_settings);
*/
    /* Check if stack initialization was successful /
    if( WICED_BT_SUCCESS == result)
    {
        printf("Bluetooth Stack Initialization Successful \n");
    }
    else
    {
        printf("Bluetooth Stack Initialization failed!! \n");
        CY_ASSERT(0);
    }
/*
    /* Create Button Task for processing button presses /
    rtos_result = xTaskCreate(button_task,"button_task", BUTTON_TASK_STACK_SIZE,
                              NULL, BUTTON_TASK_PRIORITY, &button_task_handle);
    if( pdPASS != rtos_result)
    {
        printf("Failed to create Button task! \n");
        CY_ASSERT(0);
    }

    /* Create Button Task for processing button presses /
	rtos_result = xTaskCreate(JoyStickTask, "joystick readout", configMINIMAL_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, &joystickHandle);
	if( pdPASS != rtos_result)
	{
		printf("Failed to create joystick task! \n");
		CY_ASSERT(0);
	}

	/* Create Seven segments task for processing button presses /
	rtos_result = xTaskCreate(DriveSevenSegments, "7segment", configMINIMAL_STACK_SIZE, NULL, SEVENSEGM_TASK_PRIORITY, &sevenSegmentHandle);
	if( pdPASS != rtos_result)
	{
		printf("Failed to create sevensegment task! \n");
		CY_ASSERT(0);
	}
*/
	/* Create Seven segments task for processing button presses */
	rtos_result = xTaskCreate(max7219_task, "MAX7219", MAX7219_STACK_SIZE, NULL, MAX7219_TASK_PRIORITY, NULL);
	if( pdPASS != rtos_result)
	{
		printf("Failed to create max7219 task! \n");
		CY_ASSERT(0);
	}

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* The application should never reach here */
    CY_ASSERT(0) ;
}

/* [] END OF FILE */
