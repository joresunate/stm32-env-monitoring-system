/**
 * @file bme280_i2c.h
 * @brief Low-level I2C access helpers for the BME280 sensor driver.
 *
 * This header exposes a thin HAL-based register access layer used by the
 * higher-level BME280 driver to read and write sensor registers over I2C.
 */

#ifndef BME280_I2C_H
#define BME280_I2C_H

#include <stdint.h>
#include "stm32f4xx.h"

/**
 * @brief Read one or more bytes from a BME280 register over I2C.
 *
 * @param[in] i2c Pointer to the initialized HAL I2C handle.
 * @param[in] address BME280 I2C address in HAL format.
 * @param[in] reg_addr Register address to read from.
 * @param[out] data Pointer to the destination buffer.
 * @param[in] length Number of bytes to read.
 *
 * @return 0  Read completed successfully.
 * @return -1 I2C memory read transaction failed.
 */
int BME280_I2C_Read(I2C_HandleTypeDef *i2c,
                    uint8_t address,
                    uint8_t reg_addr,
                    uint8_t *data,
                    uint16_t length);

/**
 * @brief Write one byte to a BME280 register over I2C.
 *
 * @param[in] i2c Pointer to the initialized HAL I2C handle.
 * @param[in] address BME280 I2C address in HAL format.
 * @param[in] reg_addr Register address to write to.
 * @param[in] data Data byte to write.
 *
 * @return 0  Write completed successfully.
 * @return -1 I2C memory write transaction failed.
 */
int BME280_I2C_Write(I2C_HandleTypeDef *i2c,
                     uint8_t address,
                     uint8_t reg_addr,
                     uint8_t data);

#endif
