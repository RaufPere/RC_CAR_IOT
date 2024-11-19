/*
 * motor.h
 *
 *  Created on: 19 Nov 2024
 *      Author: raufp
 */

#ifndef SOURCE_MOTOR_H_
#define SOURCE_MOTOR_H_

// define motor pins connections
#define motorPinA1 P9_1
#define motorPinA2 P9_2
#define motorPinB1 P9_4
#define motorPinB2 P9_7

// define pwm frequency motors
#define PWM_FREQUENCY 10000

// define pwm objects for motors
extern cyhal_pwm_t pwm_obj_motorA1;
extern cyhal_pwm_t pwm_obj_motorB1;
extern cyhal_pwm_t pwm_obj_motorA2;
extern cyhal_pwm_t pwm_obj_motorB2;

// define the struct for the joystick data incoming from the controller
typedef struct{
	u_int8_t x;
	u_int8_t y;
}joystickData;

// Queue handle for sending joystick data
QueueHandle_t joystickDataQueueHandle;

// define motor functions
void initMotorGPIO();
void motorDriverFunction(uint8_t forward, uint8_t rotation);
void motor_driver_task(void *pvParameters);

#endif /* SOURCE_MOTOR_H_ */
