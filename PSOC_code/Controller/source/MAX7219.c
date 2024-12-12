#include <MAX7219.h>
/*
 * Digits are the rows of 1 8x8 display...!!!
 *
 * */
// Define MAX7219 registers
#define MAX7219_NO_OP         0x00
#define MAX7219_DIGIT_0       0x01
#define MAX7219_DIGIT_1       0x02
#define MAX7219_DIGIT_2       0x03
#define MAX7219_DIGIT_3       0x04
#define MAX7219_DIGIT_4       0x05
#define MAX7219_DIGIT_5       0x06
#define MAX7219_DIGIT_6       0x07
#define MAX7219_DIGIT_7       0x08
#define MAX7219_DECODE_MODE   0x09
#define MAX7219_INTENSITY     0x0A
#define MAX7219_SCAN_LIMIT    0x0B
#define MAX7219_SHUTDOWN      0x0C
#define MAX7219_DISPLAY_TEST  0x0F

// Number of cascaded MAX7219 devices
#define NUM_MAX7219 1
cyhal_spi_t mSPI;


void handle_error(uint32_t status)
{
    if (status != CY_RSLT_SUCCESS)
    {
    	printf("Error in initializing SPI\n");
        CY_ASSERT(0);
    }
}

void max7219_task(void* param)
{
    max7219_init();           // Initialize the MAX7219
    max7219_display_blank_digits(); // Display digits across all devices
    for(;;)
    {

    }
    return;
}

// Write to all MAX7219 chips in cascade
void spi_write(cyhal_spi_t* mSPI, uint8_t address, uint8_t data)
{

	uint8_t transmit_buf[2] = {address, data};
    // Start SPI communication

	    if (CY_RSLT_SUCCESS == cyhal_spi_transfer_async(mSPI, transmit_buf, 2, NULL, 0))
	    {
	    	printf("SPI transfer was completed\n");
	    }
	    max7219_update();
    //cyhal_system_delay_ms(CMD_TO_CMD_DELAY);
}

void max7219_update(void)
{
	cyhal_gpio_write(CYBSP_SPI_CS, true);
	cyhal_gpio_write(CYBSP_SPI_CS, false);
}

// Initialize all MAX7219 devices
void max7219_init(void)
{
	printf("Configuring SPI master...\r\n");
	    /* Enable SPI to operate */
    /* Configuring the  SPI master:  Specify the SPI interface pins, frame size, SPI Motorola mode and master mode */
	cy_rslt_t rslt = cyhal_spi_init(&mSPI, CYBSP_SPI_MOSI, CYBSP_SPI_MISO, CYBSP_SPI_CLK, CYBSP_SPI_CS, NULL, 8, CYHAL_SPI_MODE_00_MSB, false);
    if(CY_RSLT_SUCCESS != rslt)
    {
    	printf("SPI init went wrong\n");
    }
	/* Set the data rate to 1 Mbps */
    rslt = cyhal_spi_set_frequency(&mSPI, SPI_FREQ_HZ);
    if(CY_RSLT_SUCCESS != rslt)
    {
    	printf("SPI init went wrong\n");
    }
    max7219_update();
	uint8_t init_data = 0xFF; // Initialize all chips
	init_data = 0xFF; // Turn on display
	spi_write(&mSPI, MAX7219_SHUTDOWN, init_data);     // Exit shutdown mode
	init_data = 0x00; // brightness
	spi_write(&mSPI,MAX7219_INTENSITY, init_data);    // Set intensity
	init_data = 0x07; // Use all digits
	spi_write(&mSPI,MAX7219_SCAN_LIMIT, init_data);   // Enable all digits
	init_data = 0x00; // Decode mode
    spi_write(&mSPI,MAX7219_DECODE_MODE, init_data);   // Enable decode mode for all digits

    printf("SPI init is done\n");
}

// Display digits on all cascaded MAX7219 chips
void max7219_display_blank_digits(void)
{
	spi_write(&mSPI,MAX7219_DIGIT_0, 0b00000000);
	spi_write(&mSPI,MAX7219_DIGIT_0, 0b00000000);
	spi_write(&mSPI,MAX7219_DIGIT_0, 0b00000000);
	spi_write(&mSPI,MAX7219_DIGIT_0, 0b00010000);

	spi_write(&mSPI,MAX7219_DIGIT_4, 0b00000000);
	spi_write(&mSPI,MAX7219_DIGIT_5, 0b00000000);
	spi_write(&mSPI,MAX7219_DIGIT_6, 0b00000000);
	spi_write(&mSPI,MAX7219_DIGIT_7, 0b00000000);


}

