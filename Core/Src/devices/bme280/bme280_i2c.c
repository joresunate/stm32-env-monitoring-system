/**
 * @file bme280_i2c.c
 * @brief HAL-based I2C register access layer for the BME280 sensor.
 *
 * This module implements simple memory read and write helpers used by the
 * BME280 driver to access sensor registers through the STM32 HAL I2C API.
 */

#include <devices/bme280/bme280.h>
#include <devices/bme280/bme280_i2c.h>

/**
 * @brief Read data from a BME280 register via I2C.
 *
 * The function performs a standard HAL memory read transaction:
 * START -> Slave Address (Write) -> Register Address ->
 * RESTART -> Slave Address (Read) -> Data -> STOP
 *
 * @param[in] i2c Pointer to the initialized HAL I2C handle.
 * @param[in] address BME280 I2C address in HAL format.
 * @param[in] reg_addr Register address to read from.
 * @param[out] data Pointer to the destination buffer.
 * @param[in] length Number of bytes to read.
 *
 * @return 0  Read completed successfully.
 * @return -1 HAL reported an error during the memory read transaction.
 */
int BME280_I2C_Read(I2C_HandleTypeDef *hi2c,
                    uint8_t address,
                    uint8_t reg_addr,
                    uint8_t *data,
                    uint16_t length)
{
    /* Perform a blocking memory read transaction through the HAL layer. */
    if (HAL_I2C_Mem_Read(hi2c,
                         address,
                         reg_addr,
                         I2C_MEMADD_SIZE_8BIT,
                         data,
                         length,
                         BME280_TIMEOUT) != HAL_OK)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Write one byte to a BME280 register via I2C.
 *
 * The function performs a standard HAL memory write transaction:
 * START -> Slave Address (Write) -> Register Address -> Data -> STOP
 *
 * @param[in] i2c Pointer to the initialized HAL I2C handle.
 * @param[in] address BME280 I2C address in HAL format.
 * @param[in] reg_addr Register address to write to.
 * @param[in] data Data byte to write.
 *
 * @return 0  Write completed successfully.
 * @return -1 HAL reported an error during the memory write transaction.
 */
int BME280_I2C_Write(I2C_HandleTypeDef *hi2c,
                     uint8_t address,
                     uint8_t reg_addr,
                     uint8_t data)
{
    /* Perform a blocking memory write transaction through the HAL layer. */
    if (HAL_I2C_Mem_Write(hi2c,
                          address,
                          reg_addr,
                          I2C_MEMADD_SIZE_8BIT,
                          &data,
                          1U,
                          BME280_TIMEOUT) != HAL_OK)
    {
        return -1;
    }

    return 0;
}
