/*
 * MAX7219.h
 *
 *  Created on: 10 dec. 2024
 *      Author: darre
 */

#ifndef SOURCE_MAX7219_H_
#define SOURCE_MAX7219_H_
#include <stdint.h>
#include <stdio.h>
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#define MAX7219_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define MAX7219_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)
/* SPI baud rate in Hz */
#define SPI_FREQ_HZ                (1000000UL)
/* Delay of 1000ms between commands */
#define CMD_TO_CMD_DELAY           (10UL)
/* SPI transfer bits per frame */
#define BITS_PER_FRAME             (8)

void handle_error(uint32_t status);
void max7219_task(void* param);
void spi_write(cyhal_spi_t* mSPI, uint8_t address, uint8_t data);
void max7219_init(void);
void max7219_display_blank_digits(void);
void max7219_update(void);


#endif /* SOURCE_MAX7219_H_ */
