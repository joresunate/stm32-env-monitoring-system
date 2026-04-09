/**
 * @file i2c1.h
 * @brief Bare-metal I2C1 master driver interface.
 *
 * This header provides a minimal register-level API for configuring I2C1 and
 * performing basic master transactions required by the project sensors and
 * display expander.
 */

#ifndef I2C1_H
#define I2C1_H

#include <stdint.h>
#include "stm32f4xx.h"

/**
 * @brief Timeout counter used by polling loops in I2C operations.
 */
#define I2C1_TIMEOUT_COUNTER  100000U

/**
 * @brief Initialize the I2C1 peripheral and its GPIO pins.
 *
 * The driver uses PB8 as SCL and PB9 as SDA in alternate function mode.
 *
 * @return None.
 */
void I2C1_Init(void);

/**
 * @brief Generate an I2C START condition and transmit the slave address.
 *
 * @param[in] address 7-bit I2C slave address.
 * @param[in] direction Transfer direction.
 *                      0 = write, 1 = read.
 *
 * @return 0  START and address phase completed successfully.
 * @return -1 I2C bus remained busy until timeout expired.
 * @return -2 START condition was not acknowledged in time.
 * @return -3 Slave address phase did not complete in time.
 * @return -4 Slave returned NACK on address phase.
 */
int I2C1_Start(uint8_t address, uint8_t direction);

/**
 * @brief Generate an I2C STOP condition.
 *
 * @return None.
 */
void I2C1_Stop(void);

/**
 * @brief Write one byte on the I2C bus.
 *
 * @param[in] data Data byte to transmit.
 *
 * @return 0  Byte transmitted successfully.
 * @return -1 TXE flag did not become ready before timeout expired.
 * @return -2 BTF flag did not become set before timeout expired.
 */
int I2C1_Write(uint8_t data);

#endif
