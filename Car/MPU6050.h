#ifndef MPU6050_H_
#define MPU6050_H_
/*
 *
 * PSOC6 (PDL)
 * PIN 6_0 = SCL
 * PIN 6_1 = SDA
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "cycfg.h"
#include "cy_pdl.h"
#include "math.h"
// Should have 100 words minimum when stack space is 512 words
#define configMINIMAL_MPU6050_STACK_SIZE                300
// ------------------------------
// Defines for MPU6050 Registers
// ------------------------------
#define I2C_MPU6050_ADDR (0x68UL)  // MPU6050 I2C address
#define I2C_REG_PWR_MGMT_1 (0x6BUL)  // Power Management 1 register
#define I2C_REG_ACCEL_XOUT_H (0x3B) // Accelerometer X-axis data high byte
#define I2C_REG_TEMP_H (0x41UL)
#define I2C_REG_TEMP_L (0x42UL)
#define I2C_REG_GYRO_XOUT_H (0x43UL)
#define I2C_REG_SMPRT_DIV (0x19UL)
#define I2C_REG_MST_CTRL (0x24UL)
#define I2C_REG_WHO_AM_I (0x75UL)

// ------------------------------
// Defines for MPU6050 constants
// ------------------------------

#define I2C_MST_CLK (13UL)
#define SENSITIVITYSCALE_GYRO 131.0
#define SENSITIVITYSCALE_ACCL 16384.0
#define TEMP_CONST_ADD 36.53
#define TEMP_CONST_DIV 340.0
#define MPU6050_SLEEP_MODE (0x40UL)
#define I2C_TIMEOUT 100UL
#define numBytesToRead 2UL

// ------------------------------
// initializing variables
// ------------------------------

// ------------------------------
// Function Prototypes
// ------------------------------
void I2C_PDL_Setup(void);
void I2C_PDL_Deinit(void);
void probe(void);
bool handle_i2c(cy_en_scb_i2c_status_t *status);
cy_en_scb_i2c_status_t I2C_readRegister(
                                        uint8_t slaveAddress,
                                        uint8_t *writeBuffer,
                                        uint8_t writeSize,
                                        uint8_t *readBuffer,
                                        uint8_t readSize);
cy_en_scb_i2c_status_t I2C_writeRegister(
                                        uint8_t slaveAddress,
                                        uint8_t *writeBuffer,
                                        uint8_t writeSize
                                        );
void MPU6050_i2c_identify();
void MPU6050_i2c_tempRead(float* temp);
bool MPU6050_i2c_config();
bool MPU6050_i2c_accelerometer(float* accel_x, float* accel_y, float* accel_z, float* magnitude);
bool MPU6050_i2c_gyroscoop(float* gyroXdps, float* gyroYdps, float* gyroZdps);

#endif /* MPU6050_H_ */
