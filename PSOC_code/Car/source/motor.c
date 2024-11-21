/*
 * motor.c
 *
 *  Created on: 19 Nov 2024
 *      Author: raufp
 */
#include "motor.h"
#include "cy_pdl.h"
#include "cyhal_gpio.h"
cyhal_pwm_t pwm_obj_motorA1; // Defined here
cyhal_pwm_t pwm_obj_motorA2; // Defined here
cyhal_pwm_t pwm_obj_motorB1; // Defined here
cyhal_pwm_t pwm_obj_motorB2; // Defined here

void initMotorGPIO()
{
	cyhal_pwm_init(&pwm_obj_motorA1, motorPinA1, NULL);
	cyhal_pwm_init(&pwm_obj_motorB1, motorPinB1, NULL);
	cyhal_pwm_init(&pwm_obj_motorA2, motorPinA2, NULL);
	cyhal_pwm_init(&pwm_obj_motorB2, motorPinB2, NULL);

	cyhal_pwm_set_duty_cycle(&pwm_obj_motorA1, 0, PWM_FREQUENCY);
	cyhal_pwm_set_duty_cycle(&pwm_obj_motorB1, 0, PWM_FREQUENCY);
	cyhal_pwm_set_duty_cycle(&pwm_obj_motorA2, 0, PWM_FREQUENCY);
	cyhal_pwm_set_duty_cycle(&pwm_obj_motorB2, 0, PWM_FREQUENCY);

	cyhal_pwm_start(&pwm_obj_motorA1);
	cyhal_pwm_start(&pwm_obj_motorB1);
	cyhal_pwm_start(&pwm_obj_motorA2);
	cyhal_pwm_start(&pwm_obj_motorB2);
}

// Function takes the two 8 bit inputs from the joystick. Forward makes the wheels go forward when between
// 127 and 255 and makes them go backwards when between 0 and 126. If rotation is 255 and forward is around 127
// then turn the car right. If rotation is 0 and forward is around 127 turn left. Car can only turn when it stands still.
void motorDriverFunction(uint8_t forward, uint8_t rotation)
{
	int dutycycle = 0;

	// Turn right at fixed speed
	if (rotation >= 240)
	{
		// Turn off opposite h bridge pair to not short anything
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA2, 0, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB1, 0, PWM_FREQUENCY);

		// Set pwm on correct h bridge pair to get movement
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA1, 30, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB2, 30, PWM_FREQUENCY);
	}
	// Turn left at fixed speed
	else if (rotation <= 15)
	{
		// Turn off opposite h bridge pair to not short anything
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA1, 0, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB2, 0, PWM_FREQUENCY);

		// Set pwm on correct h bridge pair to get movement
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA2, 30, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB1, 30, PWM_FREQUENCY);
	}
	// Drive forward
	else if (forward >= 185 && (rotation < 240 && rotation > 15))
	{
		dutycycle = (forward - 127) * 100 / (255 - 127); // maps value from 127 to 255 to a value from 0 to 100
		printf("Dutycycle forward: %d\n\r", dutycycle);

		// Turn off opposite h bridge pair to not short anything
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA2, 0, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB2, 0, PWM_FREQUENCY);

		// Set pwm on correct h bridge pair to get movement
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA1, dutycycle, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB1, dutycycle, PWM_FREQUENCY);
	}
	// Drive backwards
	else if (forward <= 180 && (rotation < 240 && rotation > 15))
	{
		dutycycle = (126 - forward) * 100 / 126;; // inversly maps value from 0 to 126 to a value from 0 to 100
		printf("Dutycycle backwards: %d\n\r", dutycycle);

		// Turn off opposite h bridge pair to not short anything
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA1, 0, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB1, 0, PWM_FREQUENCY);

		// Set pwm on correct h bridge pair to get movement
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA2, dutycycle, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB2, dutycycle, PWM_FREQUENCY);
	}
	// if none stop
	else
	{
		// Turn off
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA1, 0, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB1, 0, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorA2, 0, PWM_FREQUENCY);
		cyhal_pwm_set_duty_cycle(&pwm_obj_motorB2, 0, PWM_FREQUENCY);
	}
}

void motor_driver_task(void *pvParameters)
{
	joystickData_t inData;
	initMotorGPIO();

	for (;;)
	{
		// Read joystick data
		xQueueReceive(joystickDataQueueHandle, &inData, portMAX_DELAY);

		// Drive motors
		motorDriverFunction(inData.x, inData.y);
	}
}
