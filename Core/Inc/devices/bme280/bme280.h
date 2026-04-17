/**
 * @file bme280.h
 * @brief Public interface for the BME280 environmental sensor driver.
 *
 * This header defines the sensor register map, configuration constants,
 * data structures, and the public API used to initialize the BME280,
 * configure measurement parameters, read raw and compensated data, and
 * switch between operating modes.
 */

#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "stm32f4xx.h"

/**
 * @brief 7-bit I2C address of the BME280 when SDO is tied to GND.
 *
 * The value is shifted left by one bit to match the addressing format used
 * by the I2C helper layer in this project.
 */
#define BME280_I2C_ADDR_LOW    (0x76U << 1)

/**
 * @brief 7-bit I2C address of the BME280 when SDO is tied to VCC.
 *
 * The value is shifted left by one bit to match the addressing format used
 * by the I2C helper layer in this project.
 */
#define BME280_I2C_ADDR_HIGH   (0x77U << 1)

/**
 * @brief Fixed chip identification value returned by the BME280 ID register.
 */
#define BME280_CHIP_ID         0x60U

/**
 * @brief BME280 register addresses used by the driver.
 */
#define BME280_REG_ID          0xD0U
#define BME280_REG_RESET       0xE0U
#define BME280_REG_STATUS      0xF3U
#define BME280_REG_CTRL_HUM    0xF2U
#define BME280_REG_CTRL_MEAS   0xF4U
#define BME280_REG_CONFIG      0xF5U
#define BME280_REG_PRESS_MSB   0xF7U
#define BME280_REG_PRESS_LSB   0xF8U
#define BME280_REG_PRESS_XLSB  0xF9U
#define BME280_REG_TEMP_MSB    0xFAU
#define BME280_REG_TEMP_LSB    0xFBU
#define BME280_REG_TEMP_XLSB   0xFCU
#define BME280_REG_HUM_MSB     0xFDU
#define BME280_REG_HUM_LSB     0xFEU

/**
 * @brief Start addresses and lengths of the calibration data blocks.
 */
#define BME280_CALIB00_START   0x88U
#define BME280_CALIB00_LENGTH  26U
#define BME280_CALIB26_START   0xE1U
#define BME280_CALIB26_LENGTH  7U

/**
 * @brief Reset command value accepted by the BME280 reset register.
 */
#define BME280_RESET_VALUE     0xB6U

/**
 * @brief Measurement mode selection values for ctrl_meas[1:0].
 */
#define BME280_MODE_SLEEP      0x00U
#define BME280_MODE_FORCED     0x01U
#define BME280_MODE_NORMAL     0x03U

/**
 * @brief Oversampling configuration values for temperature, pressure,
 *        and humidity measurements.
 */
#define BME280_OSRS_SKIP       0x00U
#define BME280_OSRS_1          0x01U
#define BME280_OSRS_2          0x02U
#define BME280_OSRS_4          0x03U
#define BME280_OSRS_8          0x04U
#define BME280_OSRS_16         0x05U

/**
 * @brief Status register bit masks.
 */
#define BME280_STATUS_MEASURING  (1U << 3)
#define BME280_STATUS_IM_UPDATE  (1U << 0)

/**
 * @brief Timeout value used while waiting for sensor operations to complete.
 */
#define BME280_TIMEOUT         500U

/**
 * @brief Standby time configuration values for config.t_sb[7:5].
 */
#define BME280_T_SB_0_5_MS     (0x00U << 5)
#define BME280_T_SB_62_5_MS    (0x01U << 5)
#define BME280_T_SB_125_MS     (0x02U << 5)
#define BME280_T_SB_250_MS     (0x03U << 5)
#define BME280_T_SB_500_MS     (0x04U << 5)
#define BME280_T_SB_1000_MS    (0x05U << 5)
#define BME280_T_SB_10_MS      (0x06U << 5)
#define BME280_T_SB_20_MS      (0x07U << 5)

/**
 * @brief IIR filter configuration values for config.filter[4:2].
 */
#define BME280_FILTER_OFF      (0x00U << 2)
#define BME280_FILTER_2        (0x01U << 2)
#define BME280_FILTER_4        (0x02U << 2)
#define BME280_FILTER_8        (0x03U << 2)
#define BME280_FILTER_16       (0x04U << 2)

/**
 * @brief Calibration coefficients read from the BME280 non-volatile memory.
 */
typedef struct
{
    uint16_t dig_T1; /**< Temperature calibration coefficient T1. */
    int16_t  dig_T2; /**< Temperature calibration coefficient T2. */
    int16_t  dig_T3; /**< Temperature calibration coefficient T3. */

    uint16_t dig_P1; /**< Pressure calibration coefficient P1. */
    int16_t  dig_P2; /**< Pressure calibration coefficient P2. */
    int16_t  dig_P3; /**< Pressure calibration coefficient P3. */
    int16_t  dig_P4; /**< Pressure calibration coefficient P4. */
    int16_t  dig_P5; /**< Pressure calibration coefficient P5. */
    int16_t  dig_P6; /**< Pressure calibration coefficient P6. */
    int16_t  dig_P7; /**< Pressure calibration coefficient P7. */
    int16_t  dig_P8; /**< Pressure calibration coefficient P8. */
    int16_t  dig_P9; /**< Pressure calibration coefficient P9. */

    uint8_t  dig_H1; /**< Humidity calibration coefficient H1. */
    int16_t  dig_H2; /**< Humidity calibration coefficient H2. */
    uint8_t  dig_H3; /**< Humidity calibration coefficient H3. */
    int16_t  dig_H4; /**< Humidity calibration coefficient H4. */
    int16_t  dig_H5; /**< Humidity calibration coefficient H5. */
    int8_t   dig_H6; /**< Humidity calibration coefficient H6. */

} BME280_CalibData_t;

/**
 * @brief BME280 device handle.
 *
 * The handle stores the I2C peripheral pointer, the sensor address, cached
 * calibration coefficients, and the internal t_fine value used by the Bosch
 * compensation formulas.
 */
typedef struct
{
    I2C_HandleTypeDef *i2c;   /**< Pointer to the HAL I2C handle. */
    uint8_t address;          /**< BME280 I2C address. */
    BME280_CalibData_t calib; /**< Calibration coefficients cache. */
    int32_t t_fine;           /**< Internal compensation variable. */

} BME280_Handle_t;

/**
 * @brief Compensated environmental data in physical units.
 */
typedef struct
{
    float temperature; /**< Temperature in degrees Celsius. */
    float pressure;    /**< Pressure in hPa. */
    float humidity;    /**< Relative humidity in percent. */

} BME280_Data_t;

/**
 * @brief Sensor configuration parameters.
 */
typedef struct
{
    uint8_t osrs_t;   /**< Temperature oversampling setting. */
    uint8_t osrs_p;   /**< Pressure oversampling setting. */
    uint8_t osrs_h;   /**< Humidity oversampling setting. */

    uint8_t filter;   /**< IIR filter configuration value. */
    uint8_t standby;  /**< Standby time configuration value. */

} BME280_Config_t;

/**
 * @brief Temperature unit selection for conversion helpers.
 */
typedef enum
{
    BME280_TEMP_UNIT_C, /**< Temperature in degrees Celsius. */
    BME280_TEMP_UNIT_F  /**< Temperature in degrees Fahrenheit. */

} BME280_TempUnit_t;

/**
 * @brief Pressure unit selection for conversion helpers.
 */
typedef enum
{
    BME280_PRESS_UNIT_HPA,  /**< Pressure in hPa. */
    BME280_PRESS_UNIT_MMHG  /**< Pressure in mmHg. */

} BME280_PressUnit_t;

/**
 * @brief Humidity unit selection for conversion helpers.
 */
typedef enum
{
    BME280_HUM_UNIT_PERCENT,  /**< Relative humidity in percent. */
    BME280_HUM_UNIT_FRACTION  /**< Relative humidity as a fraction. */

} BME280_HumUnit_t;

/**
 * @brief Initialize the BME280 sensor, read calibration data, and apply the default configuration.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Initialization completed successfully.
 * @return -1 I2C transaction failed while reading the chip ID.
 * @return -2 Sensor identification value does not match BME280_CHIP_ID.
 * @return -3 Sensor reset command failed or the reset status did not complete.
 * @return -4 Calibration data could not be read from the sensor.
 * @return -5 Default configuration could not be applied.
 * @return -6 Sensor mode could not be switched to forced mode.
 *
 * @note The handle must contain valid @c i2c and @c address fields before
 *       calling this function.
 */
int BME280_Init(BME280_Handle_t *handle);

/**
 * @brief Reset the BME280 sensor using the software reset command.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Reset command accepted and internal reset completed.
 * @return -1 I2C write to the reset register failed.
 * @return -2 Sensor status could not be read while waiting for reset completion.
 * @return -3 Reset completion timed out.
 */
int BME280_Reset(BME280_Handle_t *handle);

/**
 * @brief Switch the sensor to normal mode.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Mode switch completed successfully.
 * @return -1 @c BME280_SetMode() failed.
 */
int BME280_StartNormal(BME280_Handle_t *handle);

/**
 * @brief Put the sensor into sleep mode.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Mode switch completed successfully.
 * @return -1 @c BME280_SetMode() failed.
 */
int BME280_Sleep(BME280_Handle_t *handle);

/**
 * @brief Read raw uncompensated temperature, pressure, and humidity data.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[out] adc_T Pointer to the raw temperature ADC value.
 * @param[out] adc_P Pointer to the raw pressure ADC value.
 * @param[out] adc_H Pointer to the raw humidity ADC value.
 *
 * @return 0  Raw data read successfully.
 * @return -1 I2C read of the measurement block failed.
 */
int BME280_ReadRaw(BME280_Handle_t *handle,
                   int32_t *adc_T,
                   int32_t *adc_P,
                   int32_t *adc_H);

/**
 * @brief Convert temperature from degrees Celsius to the selected unit.
 *
 * @param[in] temp_c Temperature in degrees Celsius.
 * @param[in] unit Target temperature unit.
 *
 * @return Converted temperature value.
 */
float BME280_ConvertTemperature(float temp_c, BME280_TempUnit_t unit);

/**
 * @brief Convert pressure from hPa to the selected unit.
 *
 * @param[in] press_hpa Pressure in hPa.
 * @param[in] unit Target pressure unit.
 *
 * @return Converted pressure value.
 */
float BME280_ConvertPressure(float press_hpa, BME280_PressUnit_t unit);

/**
 * @brief Convert humidity from percent to the selected unit.
 *
 * @param[in] hum_percent Relative humidity in percent.
 * @param[in] unit Target humidity unit.
 *
 * @return Converted humidity value.
 */
float BME280_ConvertHumidity(float hum_percent, BME280_HumUnit_t unit);

/**
 * @brief Read compensated temperature, pressure, and humidity values.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[out] data Pointer to the output data structure.
 *
 * @return 0  Data read and compensation completed successfully.
 * @return -1 Raw measurement data could not be read from the sensor.
 */
int BME280_ReadAll(BME280_Handle_t *handle, BME280_Data_t *data);

/**
 * @brief Perform one forced measurement and read the compensated result.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[out] data Pointer to the output data structure.
 *
 * @return 0  Forced measurement completed successfully.
 * @return -1 Forced measurement could not be started or completed.
 */
int BME280_ReadForced(BME280_Handle_t *handle, BME280_Data_t *data);

/**
 * @brief Wait for the current normal-mode measurement to complete and read the compensated result.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[out] data Pointer to the output data structure.
 *
 * @return 0  Measurement completed and data were read successfully.
 * @return -1 Measurement did not complete successfully.
 */
int BME280_ReadNormal(BME280_Handle_t *handle, BME280_Data_t *data);

/**
 * @brief Set the operating mode of the sensor.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] mode Target mode value.
 *
 * @return 0  Mode updated successfully.
 * @return -1 Reading ctrl_meas failed.
 * @return -2 Writing ctrl_meas failed.
 */
int BME280_SetMode(BME280_Handle_t *handle, uint8_t mode);

/**
 * @brief Set the standby time field in the configuration register.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] t_sb Standby time field value.
 *
 * @return 0  Standby time updated successfully.
 * @return -1 Reading config failed.
 * @return -2 Writing config failed.
 */
int BME280_SetStandby(BME280_Handle_t *handle, uint8_t t_sb);

/**
 * @brief Configure oversampling for temperature, pressure, and humidity.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] osrs_t Temperature oversampling value.
 * @param[in] osrs_p Pressure oversampling value.
 * @param[in] osrs_h Humidity oversampling value.
 *
 * @return 0  Oversampling configuration updated successfully.
 * @return -1 Writing ctrl_hum failed.
 * @return -2 Reading ctrl_meas failed.
 * @return -3 Writing ctrl_meas failed.
 */
int BME280_SetOversampling(BME280_Handle_t *handle,
                           uint8_t osrs_t,
                           uint8_t osrs_p,
                           uint8_t osrs_h);

/**
 * @brief Configure the IIR filter coefficient.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] filter Filter configuration value.
 *
 * @return 0  Filter configuration updated successfully.
 * @return -1 Reading config failed.
 * @return -2 Writing config failed.
 */
int BME280_SetFilter(BME280_Handle_t *handle, uint8_t filter);

/**
 * @brief Apply a complete sensor configuration in a safe sequence.
 *
 * The function stores the current operating mode, switches the sensor to
 * sleep mode, applies oversampling and filter settings, updates the standby
 * time, and finally restores the previous mode.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] cfg Pointer to the configuration structure.
 *
 * @return 0  Configuration applied successfully.
 * @return -1 Reading the current operating mode failed.
 * @return -2 Switching the sensor to sleep mode failed.
 * @return -3 Oversampling configuration failed.
 * @return -4 Filter configuration failed.
 * @return -5 Standby configuration failed.
 * @return -6 Restoring the previous operating mode failed.
 */
int BME280_Configure(BME280_Handle_t *handle, const BME280_Config_t *cfg);

#endif
