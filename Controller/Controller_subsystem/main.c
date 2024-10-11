#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "FreeRTOS.h"
#include "task.h"

cyhal_adc_t adcObj;
cyhal_adc_channel_t adc_chan_0_obj;

cyhal_adc_channel_t adc_chan_0_obj1;

TaskHandle_t joystickHandle;

void JoyStickTask(void * parameters)
{
	int32_t adc_outX = 0;
	int32_t adc_outY = 0;

	for (;;)
	{
		adc_outX = cyhal_adc_read(&adc_chan_0_obj)-1000;

		if (adc_outX > -120 && adc_outX < 120)
		{
			adc_outX = 0;
		}

		if (adc_outX > 0)
		{
			adc_outX = adc_outX * 3;
		}

		adc_outX = adc_outX * -1;

		printf("ADC value X: %d\r\n", adc_outX);

		adc_outY = cyhal_adc_read(&adc_chan_0_obj1)-1000;

		if (adc_outY > -120 && adc_outY < 120)
		{
			adc_outY = 0;
		}

		if (adc_outY > 0)
		{
			adc_outY = adc_outY * 3;
		}

		adc_outY = adc_outY * -1;

		printf("ADC value Y: %d\r\n", adc_outY);

		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void initADC()
{
	// INIT ADC for joystick
	cy_rslt_t result = cyhal_adc_init(&adcObj, P10_2, NULL);

	if (result != CY_RSLT_SUCCESS)
	{
		printf("Init adc failed\r\n");
	}

	// Initialize ADC channel, allocate channel number 0 to pin ADC_VPLUS1 as this is the first
	// channel initialized
	const cyhal_adc_channel_config_t channel_config =
		{ .enable_averaging = false, .min_acquisition_ns = 220, .enabled = true };
	result = cyhal_adc_channel_init_diff(&adc_chan_0_obj, &adcObj, P10_2, CYHAL_ADC_VNEG,
									   &channel_config);

	// Initialize ADC channel, allocate channel number 0 to pin ADC_VPLUS1 as this is the first
	// channel initialized
	const cyhal_adc_channel_config_t channel_config1 =
		{ .enable_averaging = false, .min_acquisition_ns = 220, .enabled = true };
	result = cyhal_adc_channel_init_diff(&adc_chan_0_obj1, &adcObj, P10_4, CYHAL_ADC_VNEG,
									   &channel_config1);

	if (result != CY_RSLT_SUCCESS)
	{
		printf("Init adc channel failed 2\r\n");
	}
}

int main(void)
{
    cybsp_init();

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init_fc(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
            CYBSP_DEBUG_UART_CTS,CYBSP_DEBUG_UART_RTS,CY_RETARGET_IO_BAUDRATE);

    // Init adc
    initADC();

    xTaskCreate(JoyStickTask, "joystick readout", configMINIMAL_STACK_SIZE, NULL, 1, &joystickHandle);
    vTaskStartScheduler();
}
