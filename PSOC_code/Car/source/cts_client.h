/******************************************************************************
* File Name: ctc_client.c
*
* Description: This file contains macros, structures, enumerations and function
*              prototypes used in cts_client.c file.
*
* Related Document: See README.md
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

/*******************************************************************************
*        Header Files
*******************************************************************************/
#include "wiced_bt_dev.h"
#include <FreeRTOS.h>
#include <task.h>

/*******************************************************************************
*        Macro Definitions
*******************************************************************************/
/* Macros for button interrupt and button task */
/* Interrupt priority for the GPIO connected to the user button */
#define BUTTON_INTERRUPT_PRIORITY          (7u)
#define BLUETOOTH_TASK_PRIORITY            (configMAX_PRIORITIES - 1)
#define BLUETOOTH_TASK_STACK_SIZE          (configMINIMAL_STACK_SIZE * 12)
/*
 * // Get the stack high-water mark
        UBaseType_t high_water_mark = uxTaskGetStackHighWaterMark(NULL);

        // High-water mark is returned in words, convert to bytes
        printf("Minimum stack remaining: %u bytes\n",
               high_water_mark * sizeof(StackType_t));
 * */
#define LED_PIN P13_7
#define UUID_SERVICE_CAR                                    0x50, 0xAB, 0xC7, 0xEB, 0x64, 0x55, 0x21, 0xA5, 0x3B, 0x40, 0x1B, 0x64, 0x4F, 0xAA, 0xE3, 0x78
#define UUID_CHARACTERISTIC_CAR_JOYSTICK                    0xEC, 0xC7, 0x84, 0x0A, 0x49, 0xFF, 0xCF, 0xAE, 0x93, 0x4D, 0xA8, 0x7F, 0xE9, 0x05, 0x80, 0x02
#define UUID_CHARACTERISTIC_CAR_SPEED                       0x9D, 0xA1, 0x7A, 0xEB, 0xF4, 0x27, 0x2A, 0xB8, 0xE5, 0x49, 0x9F, 0x11, 0xC3, 0xF4, 0x76, 0x7A

/* Service CAR */
#define HDLS_CAR                                              0x0007
/* Characteristic Joystick */
#define HDLC_CAR_JOYSTICK                                     0x0008
#define HDLC_CAR_JOYSTICK_VALUE                               0x0009
#define MAX_LEN_CAR_JOYSTICK                                  0x0002
/* Descriptor Client Characteristic Configuration */
#define HDLD_CAR_JOYSTICK_CLIENT_CHAR_CONFIG                  0x000A
#define MAX_LEN_CAR_JOYSTICK_CLIENT_CHAR_CONFIG               0x0002
/* Characteristic Speed */
#define HDLC_CAR_SPEED                                        0x000B
#define HDLC_CAR_SPEED_VALUE                                  0x000C
#define MAX_LEN_CAR_SPEED                                     0x0001
/* Descriptor Client Characteristic Configuration */
#define HDLD_CAR_SPEED_CLIENT_CHAR_CONFIG                     0x000D
#define MAX_LEN_CAR_SPEED_CLIENT_CHAR_CONFIG                  0x0002
#define START_HANDLE 0x0001
#define END_HANDLE 0xFFFF
/*******************************************************************************
*        Enumerations
*******************************************************************************/

/*******************************************************************************
*        Structures
*******************************************************************************/

typedef struct
{
    uint16_t car_start_handle;
    uint16_t car_end_handle;
    uint16_t car_char_joystick_handle;
    uint16_t car_char_joystick_val_handle;
    uint16_t car__joystick_cccd_handle;
    uint16_t car_char_speed_handle;
    uint16_t car_char_speed_val_handle;
    uint16_t car_speed_cccd_handle;
    bool car_service_found;
} car_discovery_data_t;
/*******************************************************************************
 * Extern variables
 ******************************************************************************/
extern TaskHandle_t  bluetooth_task_handle;

/*******************************************************************************
*        Function Prototypes
*******************************************************************************/
/* FreeRTOS task functions */
void bluetooth_task (void *pvParameters);

/* Callback function for Bluetooth stack management events */
wiced_bt_dev_status_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                                 wiced_bt_management_evt_data_t *p_event_data);
