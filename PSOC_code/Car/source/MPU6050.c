/*
 * MPU6050.c
 *
 *  Created on: 5 nov. 2024
 *      Author: darre
 */

#include <MPU6050.h>
#define BUFFER_SIZE 200UL
char buffer[BUFFER_SIZE];
cy_stc_scb_i2c_master_xfer_config_t transfer;
cy_stc_scb_i2c_context_t I2CBUS_context;

// Absolute gyroscope values
float gyro_x_abs = 0;
float gyro_y_abs = 0;
float gyro_z_abs = 0;

void I2C_PDL_Setup(){
	 /* Set up the device based on configurator selections */
	 Cy_SCB_I2C_Init(I2CBUS_HW,&I2CBUS_config,&I2CBUS_context);
	 Cy_SCB_I2C_Enable(I2CBUS_HW);
}

void I2C_PDL_Deinit(){
	// Disable the I2C peripheral
	Cy_SCB_I2C_Disable(I2CBUS_HW, &I2CBUS_context);

	// Deinitialize the I2C configuration
	Cy_SCB_I2C_DeInit(I2CBUS_HW);
}

bool handle_i2c(cy_en_scb_i2c_status_t *status){
	if((*status) == CY_SCB_I2C_SUCCESS){
		return true;
	}
	else return false;
}

// Print the probe table

void probe()
{
    uint32_t rval;

    printf("I2C Detect\n\r");
    // Setup the screen and print the header
    printf("\n\n\r   ");
    for(unsigned int i=0;i<0x10;i++)
    {
    	printf("%02X ",i);
    }
    // Iterate through the address starting at 0x00
    for(uint32_t i2caddress=0;i2caddress<0x80;i2caddress++)
    {
        if(i2caddress % 0x10 == 0 ){
        	printf("\r\n%02X ",(unsigned int)i2caddress);
        }

        rval = Cy_SCB_I2C_MasterSendStart(I2CBUS_HW,i2caddress,CY_SCB_I2C_WRITE_XFER,10,&I2CBUS_context);
        if(rval == CY_SCB_I2C_SUCCESS ) // If you get ACK then print the address
        {
        	printf("%02X ", (unsigned int)i2caddress);
        }
        else //  Otherwise print a --
        {
        	printf("-- ");
        }
        Cy_SCB_I2C_MasterSendStop(I2CBUS_HW,0,&I2CBUS_context);
    }
    printf("\n");
}

// Basic write/read functions
cy_en_scb_i2c_status_t I2C_readRegister(
                                        uint8_t slaveAddress,
                                        uint8_t *writeBuffer,
                                        uint8_t writeSize,
                                        uint8_t *readBuffer,
                                        uint8_t readSize)
{
    cy_en_scb_i2c_status_t status;
    // Send Start condition, address and receive ACK/NACK response from slave
    status = Cy_SCB_I2C_MasterSendStart(I2CBUS_HW, slaveAddress, CY_SCB_I2C_WRITE_XFER, I2C_TIMEOUT, &I2CBUS_context);
    if (status == CY_SCB_I2C_SUCCESS)
    {
        uint32_t cnt = 0UL;
        // Write the data into the slave from the buffer
        while ((status == CY_SCB_I2C_SUCCESS) && (cnt < writeSize))
        {
            status = Cy_SCB_I2C_MasterWriteByte(I2CBUS_HW, writeBuffer[cnt], I2C_TIMEOUT, &I2CBUS_context);
            ++cnt;
        }
    }

    // Check status of the write transaction
    if ((status == CY_SCB_I2C_SUCCESS) ||
        (status == CY_SCB_I2C_MASTER_MANUAL_NAK) ||
        (status == CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK))
    {
        // Send ReStart condition for the burst read transaction
        status = Cy_SCB_I2C_MasterSendReStart(I2CBUS_HW, slaveAddress, CY_SCB_I2C_READ_XFER, I2C_TIMEOUT, &I2CBUS_context);
        if (status == CY_SCB_I2C_SUCCESS)
        {
            uint32_t cnt = 0UL;
            // Read bytes from the slave
            while ((status == CY_SCB_I2C_SUCCESS) && (cnt < readSize))
            {
                // Read byte from slave with ACK, except for the last byte with NAK
                cy_en_scb_i2c_command_t ackNak = (cnt == (readSize - 1)) ? CY_SCB_I2C_NAK : CY_SCB_I2C_ACK;
                status = Cy_SCB_I2C_MasterReadByte(I2CBUS_HW, ackNak, &readBuffer[cnt], I2C_TIMEOUT, &I2CBUS_context);
                ++cnt;
            }

            // Send Stop condition if the read was successful
            if (status == CY_SCB_I2C_SUCCESS)
            {
                status = Cy_SCB_I2C_MasterSendStop(I2CBUS_HW, I2C_TIMEOUT, &I2CBUS_context);
            }
        }
    }

    return status;
}

cy_en_scb_i2c_status_t I2C_writeRegister(
                                        uint8_t slaveAddress,
                                        uint8_t *writeBuffer,
                                        uint8_t writeSize
                                        ){
		cy_en_scb_i2c_status_t status;
	    // Send Start condition, address and receive ACK/NACK response from slave
	    status = Cy_SCB_I2C_MasterSendStart(I2CBUS_HW, slaveAddress, CY_SCB_I2C_WRITE_XFER, I2C_TIMEOUT, &I2CBUS_context);
	    if (status == CY_SCB_I2C_SUCCESS)
	    {
	        uint32_t cnt = 0UL;
	        // Write the data into the slave from the writeBuffer
	        while ((status == CY_SCB_I2C_SUCCESS) && (cnt < writeSize))
	        {
	            status = Cy_SCB_I2C_MasterWriteByte(I2CBUS_HW, writeBuffer[cnt], I2C_TIMEOUT, &I2CBUS_context);
	            ++cnt;
	        }
	    }
	    return Cy_SCB_I2C_MasterSendStop(I2CBUS_HW, I2C_TIMEOUT, &I2CBUS_context);
}

// Reads the I2C address from the internal register of the MPU6050 (to be sure the I2C component works)
void MPU6050_i2c_identify()
{
	uint8_t readBuf[1UL];
	uint8_t writeBuf[1UL] = {I2C_REG_WHO_AM_I};
	cy_en_scb_i2c_status_t status = I2C_readRegister(I2C_MPU6050_ADDR, writeBuf, sizeof(writeBuf), readBuf, sizeof(readBuf));

	if (status == CY_SCB_I2C_SUCCESS){
    	printf("Address from MPU6050: 0x%02x\n\r", readBuf[0]);

    }
    else {
    	printf("Failed to read initial address from MPU6050 \n\r");
    }

}

void MPU6050_i2c_tempRead(float * temp){
	uint8_t readBuffer[2UL];
	uint8_t regToBeRead[1UL] = {I2C_REG_TEMP_H};
	I2C_readRegister(I2C_MPU6050_ADDR, regToBeRead, sizeof(regToBeRead), readBuffer, sizeof(readBuffer));
	int16_t temp_out_byte = (readBuffer[0] << 8) | readBuffer[1];
	*(temp) = ((temp_out_byte)/TEMP_CONST_DIV) + TEMP_CONST_ADD;
	//printf("Temp : %.2f°C\n\r", *(temp));
}

bool MPU6050_i2c_config(){
	cy_en_scb_i2c_status_t status_i2c;
	/*
	 * 1. First, read the power management register to possibly get the MPU6050 of sleep mode!
	 * 2. Second, make the sample rate of the gyroscope the same as the accelerometer (1KHz)!
	 *
	 */
	// First stage (power register)
	uint8_t readBuffer[1UL];
	uint8_t writeBuffer[2UL] = {I2C_REG_PWR_MGMT_1, 0x01UL};
	uint8_t regToBeRead[1UL] = {I2C_REG_PWR_MGMT_1};

	status_i2c = I2C_readRegister(I2C_MPU6050_ADDR, regToBeRead, sizeof(regToBeRead), readBuffer, sizeof(readBuffer));
	uint8_t sleepBit = (readBuffer[0] & 0x40); // Mask to check the 7th bit
	bool SleepBitSet = (sleepBit != 0); // True if the 7th bit is 1
	if(SleepBitSet){
		// Device has been turned off -> sleep mode is active
		// Deactivate sleep mode and select a better clock
		status_i2c = I2C_writeRegister(I2C_MPU6050_ADDR, writeBuffer, sizeof(writeBuffer));
		writeBuffer[0UL] = I2C_REG_SMPRT_DIV;
		writeBuffer[1UL] = 7UL;
		regToBeRead[0UL] = I2C_REG_SMPRT_DIV;
		status_i2c = I2C_writeRegister(I2C_MPU6050_ADDR, writeBuffer, sizeof(writeBuffer));

	}
	return handle_i2c(&status_i2c);
}

bool MPU6050_i2c_accelerometer(float* accel_x, float* accel_y, float* accel_z, float* magnitude){
	// One axis has 2 internal registers. 6x burst read for x, y and z plane.

	uint8_t readBuffer[6];
	cy_en_scb_i2c_status_t status_i2c;
	uint8_t regToBeRead[1UL] = {I2C_REG_ACCEL_XOUT_H};
	status_i2c = I2C_readRegister(I2C_MPU6050_ADDR, regToBeRead, sizeof(regToBeRead), readBuffer, sizeof(readBuffer));

	// Combine the high and low bytes for each axis
	int16_t rawX = readBuffer[0] << 8 | readBuffer[1];
	int16_t rawY = readBuffer[2] << 8 | readBuffer[3];
	int16_t rawZ = readBuffer[4] << 8 | readBuffer[5];



	// Get the scale of a read operation to have more precision.
	*(accel_x) = rawX / SENSITIVITYSCALE_ACCL;
	*(accel_y) = rawY / SENSITIVITYSCALE_ACCL;
	*(accel_z) = rawZ / SENSITIVITYSCALE_ACCL;
	*(magnitude) = sqrt(*(accel_x) * *(accel_x) + *(accel_y) * *(accel_y) + *(accel_z) * *(accel_z));
	// these values represent acceleration in the respective directions, typically in units of "g" (gravity, where 1g = 9.8 m/s²)
	//printf("\n\raccelerometer X-AXIS : %.2f \n\raccelerometer Y-AXIS : %.2f \n\raccelerometer Z-AXIS : %.2f\n\rmagnitude : %.2f\n\r", accelX_g, accelY_g, accelZ_g, magnitude);
	return handle_i2c(&status_i2c);
}

bool MPU6050_i2c_gyroscoop(float* gyroXdps, float* gyroYdps, float* gyroZdps, float* gyroXdpsABS, float* gyroYdpsABS, float* gyroZdpsABS){
	// One axis has 2 internal registers. 6x burst read for x, y and z plane.
	cy_en_scb_i2c_status_t status_i2c;
	uint8_t readBuffer[6UL];
	uint8_t regToBeRead[1UL] = {I2C_REG_GYRO_XOUT_H};
	status_i2c = I2C_readRegister(I2C_MPU6050_ADDR, regToBeRead, sizeof(regToBeRead), readBuffer, sizeof(readBuffer));
	int16_t rawX = readBuffer[0] << 8 | readBuffer[1];
	int16_t rawY = readBuffer[2] << 8 | readBuffer[3];
	int16_t rawZ = readBuffer[4] << 8 | readBuffer[5];
	*(gyroXdps) = rawX / SENSITIVITYSCALE_GYRO;
	*(gyroYdps) = rawY / SENSITIVITYSCALE_GYRO;
	*(gyroZdps) = rawZ / SENSITIVITYSCALE_GYRO;
	//printf("\n\rGYRO X-AXIS : %.2f\n\rGYRO Y-AXIS : %.2f\n\rGYRO Z-AXIS : %.2f \n\r", gyroXdps, gyroYdps, gyroZdps);

	//Update absolute values from raw read in.
	if (*gyroXdps < 5 && *gyroXdps > -5)
	{
		*gyroXdps = 0;
	}
	if (*gyroYdps < 5 && *gyroYdps > -5)
	{
		*gyroYdps = 0;
	}
	if (*gyroZdps < 5 && *gyroZdps > -5)
	{
		*gyroZdps = 0;
	}

	gyro_x_abs += *gyroXdps;
	gyro_y_abs += *gyroYdps;
	gyro_z_abs += *gyroZdps;

	*(gyroXdpsABS) = gyro_x_abs;
	*(gyroYdpsABS) = gyro_y_abs;
	*(gyroZdpsABS) = gyro_z_abs;

	return handle_i2c(&status_i2c);
}


