/**
 * @file bme280.c
 * @brief Implementation of the BME280 environmental sensor driver.
 *
 * This module provides low-level register access helpers, calibration data
 * loading, status polling, fixed-point compensation formulas, and public
 * routines for reading temperature, pressure, and humidity values from the
 * BME280 sensor over I2C.
 */

#include <devices/bme280/bme280.h>
#include <devices/bme280/bme280_i2c.h>
#include <platform/dwt_delay.h>

/* -------------------------------------------------------------------------- */
/* Low-level register access                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read one or more bytes from a BME280 register.
 *
 * This is a thin wrapper around the project-specific I2C register read helper.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] reg Register address to read from.
 * @param[out] data Pointer to the destination buffer.
 * @param[in] len Number of bytes to read.
 *
 * @return 0  Read completed successfully.
 * @return Negative value returned by the I2C helper on bus error.
 */
static int BME280_ReadRegister(BME280_Handle_t *handle,
                               uint8_t reg,
                               uint8_t *data,
                               uint16_t len)
{
    return BME280_I2C_Read(handle->i2c, handle->address, reg, data, len);
}

/**
 * @brief Write one byte to a BME280 register.
 *
 * This is a thin wrapper around the project-specific I2C register write helper.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] reg Register address to write to.
 * @param[in] data Value to write.
 *
 * @return 0  Write completed successfully.
 * @return Negative value returned by the I2C helper on bus error.
 */
static int BME280_WriteRegister(BME280_Handle_t *handle,
                                uint8_t reg,
                                uint8_t data)
{
    return BME280_I2C_Write(handle->i2c, handle->address, reg, data);
}

/* -------------------------------------------------------------------------- */
/* Calibration handling                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Read and decode all calibration coefficients from the sensor.
 *
 * The BME280 stores temperature and pressure calibration coefficients in one
 * block and humidity coefficients in a second block. This function reads both
 * blocks and decodes them into the handle cache.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Calibration data were read and decoded successfully.
 * @return -1 Reading the primary calibration block failed.
 * @return -2 Reading the humidity calibration block failed.
 */
static int BME280_ReadCalibration(BME280_Handle_t *handle)
{
    uint8_t buf1[BME280_CALIB00_LENGTH];
    uint8_t buf2[BME280_CALIB26_LENGTH];

    if (BME280_ReadRegister(handle, BME280_CALIB00_START, buf1, sizeof(buf1)) != 0)
    {
        return -1;
    }

    if (BME280_ReadRegister(handle, BME280_CALIB26_START, buf2, sizeof(buf2)) != 0)
    {
        return -2;
    }

    /* Decode temperature calibration coefficients. */
    handle->calib.dig_T1 = (uint16_t)(buf1[1] << 8 | buf1[0]);
    handle->calib.dig_T2 = (int16_t)(buf1[3] << 8 | buf1[2]);
    handle->calib.dig_T3 = (int16_t)(buf1[5] << 8 | buf1[4]);

    /* Decode pressure calibration coefficients. */
    handle->calib.dig_P1 = (uint16_t)(buf1[7] << 8 | buf1[6]);
    handle->calib.dig_P2 = (int16_t)(buf1[9] << 8 | buf1[8]);
    handle->calib.dig_P3 = (int16_t)(buf1[11] << 8 | buf1[10]);
    handle->calib.dig_P4 = (int16_t)(buf1[13] << 8 | buf1[12]);
    handle->calib.dig_P5 = (int16_t)(buf1[15] << 8 | buf1[14]);
    handle->calib.dig_P6 = (int16_t)(buf1[17] << 8 | buf1[16]);
    handle->calib.dig_P7 = (int16_t)(buf1[19] << 8 | buf1[18]);
    handle->calib.dig_P8 = (int16_t)(buf1[21] << 8 | buf1[20]);
    handle->calib.dig_P9 = (int16_t)(buf1[23] << 8 | buf1[22]);

    handle->calib.dig_H1 = buf1[25];

    /* Decode humidity calibration coefficients. */
    handle->calib.dig_H2 = (int16_t)(buf2[1] << 8 | buf2[0]);
    handle->calib.dig_H3 = buf2[2];
    handle->calib.dig_H4 = (int16_t)((buf2[3] << 4) | (buf2[4] & 0x0FU));
    handle->calib.dig_H5 = (int16_t)((buf2[5] << 4) | (buf2[4] >> 4));
    handle->calib.dig_H6 = (int8_t)buf2[6];

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Measurement control                                                        */
/* -------------------------------------------------------------------------- */

/**
 * @brief Wait until the current measurement or NVM update is completed.
 *
 * The function polls the BME280 status register until the measuring bit clears
 * or until the timeout expires.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  The operation completed successfully.
 * @return -1 Status register could not be read.
 * @return -2 Timeout expired before the operation completed.
 */
static int BME280_WaitMeasurement(BME280_Handle_t *handle)
{
    uint8_t status;
    uint32_t timeout = BME280_TIMEOUT;

    do
    {
        if (BME280_ReadRegister(handle, BME280_REG_STATUS, &status, 1) != 0)
        {
            return -1;
        }

        if (timeout-- == 0U)
        {
            return -2;
        }

    } while (status & BME280_STATUS_MEASURING);

    return 0;
}

/**
 * @brief Trigger one forced measurement and wait for completion.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Measurement completed successfully.
 * @return -1 Mode switch to forced mode failed.
 * @return -2 Measurement did not complete before the timeout expired.
 */
static int BME280_ForceMeasurement(BME280_Handle_t *handle)
{
    /* Switch the sensor into forced mode to start a single conversion cycle. */
    if (BME280_SetMode(handle, BME280_MODE_FORCED) != 0)
    {
        return -1;
    }

    return BME280_WaitMeasurement(handle);
}

/* -------------------------------------------------------------------------- */
/* Compensation formulas                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Compensate raw temperature using Bosch fixed-point formulas.
 *
 * The function updates the internal @c t_fine value required by the pressure
 * and humidity compensation formulas.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] adc_T Raw uncompensated temperature value.
 *
 * @return Compensated temperature value in centi-degrees Celsius.
 */
static int32_t BME280_CompensateTemperature(BME280_Handle_t *handle,
                                            int32_t adc_T)
{
    int32_t var1, var2;

    var1 = ((((adc_T >> 3) - ((int32_t)handle->calib.dig_T1 << 1))) *
            ((int32_t)handle->calib.dig_T2)) >> 11;

    var2 = (((((adc_T >> 4) - ((int32_t)handle->calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)handle->calib.dig_T1))) >> 12) *
            ((int32_t)handle->calib.dig_T3)) >> 14;

    handle->t_fine = var1 + var2;

    return (handle->t_fine * 5 + 128) >> 8;
}

/**
 * @brief Compensate raw pressure using Bosch fixed-point formulas.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] adc_P Raw uncompensated pressure value.
 *
 * @return Compensated pressure value in the internal fixed-point format used
 *         by this module.
 *
 * @note A zero return value also indicates protection against division by zero
 *       when the calibration coefficient P1 is invalid.
 */
static uint32_t BME280_CompensatePressure(BME280_Handle_t *handle,
                                          int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)handle->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)handle->calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)handle->calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)handle->calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)handle->calib.dig_P3) >> 8) +
           ((var1 * (int64_t)handle->calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) *
           ((int64_t)handle->calib.dig_P1) >> 33;

    if (var1 == 0)
    {
        return 0U;
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;

    var1 = (((int64_t)handle->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)handle->calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) +
        (((int64_t)handle->calib.dig_P7) << 4);

    return (uint32_t)p;
}

/**
 * @brief Compensate raw humidity using Bosch fixed-point formulas.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] adc_H Raw uncompensated humidity value.
 *
 * @return Compensated humidity value in the internal fixed-point format used
 *         by this module.
 */
static uint32_t BME280_CompensateHumidity(BME280_Handle_t *handle,
                                          int32_t adc_H)
{
    int32_t v_x1_u32r;

    v_x1_u32r = handle->t_fine - ((int32_t)76800);

    v_x1_u32r = (((((adc_H << 14) -
                    (((int32_t)handle->calib.dig_H4) << 20) -
                    (((int32_t)handle->calib.dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r *
                        ((int32_t)handle->calib.dig_H6)) >> 10) *
                      (((v_x1_u32r *
                         ((int32_t)handle->calib.dig_H3)) >> 11) +
                       ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) *
                       ((int32_t)handle->calib.dig_H2) + 8192) >> 14));

    v_x1_u32r = v_x1_u32r -
                (((((v_x1_u32r >> 15) *
                    (v_x1_u32r >> 15)) >> 7) *
                  ((int32_t)handle->calib.dig_H1)) >> 4);

    if (v_x1_u32r < 0)
    {
        v_x1_u32r = 0;
    }

    if (v_x1_u32r > 419430400)
    {
        v_x1_u32r = 419430400;
    }

    return (uint32_t)(v_x1_u32r >> 12);
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the BME280 sensor and load the default configuration.
 *
 * The function verifies the chip ID, performs a software reset, reads
 * calibration data, applies the default oversampling and filter settings,
 * and finally switches the sensor into forced mode.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Initialization completed successfully.
 * @return -1 Chip ID could not be read.
 * @return -2 Chip ID does not match BME280_CHIP_ID.
 * @return -3 Reset failed.
 * @return -4 Calibration data could not be read.
 * @return -5 Default configuration could not be applied.
 * @return -6 Sensor could not be switched to forced mode.
 */
int BME280_Init(BME280_Handle_t *handle)
{
    uint8_t id;

    /* Verify that the connected device is a BME280. */
    if (BME280_ReadRegister(handle, BME280_REG_ID, &id, 1) != 0)
    {
        return -1;
    }

    if (id != BME280_CHIP_ID)
    {
        return -2;
    }

    /* Reset the sensor before loading calibration data and settings. */
    if (BME280_Reset(handle) != 0)
    {
        return -3;
    }

    /* Read factory calibration coefficients into the local cache. */
    if (BME280_ReadCalibration(handle) != 0)
    {
        return -4;
    }

    /* Default configuration used by the project. */
    BME280_Config_t cfg =
    {
        .osrs_t = BME280_OSRS_2,
        .osrs_p = BME280_OSRS_4,
        .osrs_h = BME280_OSRS_1,
        .filter = BME280_FILTER_4,
        .standby = BME280_T_SB_500_MS
    };

    if (BME280_Configure(handle, &cfg) != 0)
    {
        return -5;
    }

    /* Start in forced mode so that the first acquisition can be triggered on demand. */
    if (BME280_SetMode(handle, BME280_MODE_FORCED) != 0)
    {
        return -6;
    }

    return 0;
}

/**
 * @brief Perform a software reset of the BME280 sensor.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Reset command accepted and completion detected.
 * @return -1 Reset command could not be written.
 * @return -2 Sensor status could not be read while waiting for completion.
 * @return -3 Reset completion timed out.
 */
int BME280_Reset(BME280_Handle_t *handle)
{
    if (BME280_WriteRegister(handle, BME280_REG_RESET, BME280_RESET_VALUE) != 0)
    {
        return -1;
    }

    /* Give the device time to start processing the reset command. */
    DWT_DelayMs(3);

    uint8_t status;
    uint32_t timeout = BME280_TIMEOUT;

    do
    {
        if (BME280_ReadRegister(handle, BME280_REG_STATUS, &status, 1) != 0)
        {
            return -2;
        }

        if (timeout-- == 0U)
        {
            return -3;
        }

    } while (status & BME280_STATUS_IM_UPDATE);

    return 0;
}

/**
 * @brief Switch the sensor into normal mode.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Mode switch completed successfully.
 * @return Negative value returned by @c BME280_SetMode().
 */
int BME280_StartNormal(BME280_Handle_t *handle)
{
    return BME280_SetMode(handle, BME280_MODE_NORMAL);
}

/**
 * @brief Switch the sensor into sleep mode.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 *
 * @return 0  Mode switch completed successfully.
 * @return Negative value returned by @c BME280_SetMode().
 */
int BME280_Sleep(BME280_Handle_t *handle)
{
    return BME280_SetMode(handle, BME280_MODE_SLEEP);
}

/**
 * @brief Read the raw measurement block from the sensor.
 *
 * The measurement block contains pressure, temperature, and humidity raw ADC
 * values packed in the order defined by the datasheet.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[out] adc_T Pointer to the raw temperature result.
 * @param[out] adc_P Pointer to the raw pressure result.
 * @param[out] adc_H Pointer to the raw humidity result.
 *
 * @return 0  Raw data read successfully.
 * @return -1 I2C read of the measurement block failed.
 */
int BME280_ReadRaw(BME280_Handle_t *handle,
                   int32_t *adc_T,
                   int32_t *adc_P,
                   int32_t *adc_H)
{
    uint8_t data[8];

    if (BME280_ReadRegister(handle, BME280_REG_PRESS_MSB, data, 8) != 0)
    {
        return -1;
    }

    /* Pressure is stored in a 20-bit field across three bytes. */
    *adc_P = (int32_t)((data[0] << 12) |
                       (data[1] << 4) |
                       (data[2] >> 4));

    /* Temperature is stored in a 20-bit field across three bytes. */
    *adc_T = (int32_t)((data[3] << 12) |
                       (data[4] << 4) |
                       (data[5] >> 4));

    /* Humidity is stored as a 16-bit field across two bytes. */
    *adc_H = (int32_t)((data[6] << 8) |
                       data[7]);

    return 0;
}

/**
 * @brief Convert a temperature value from Celsius to the selected unit.
 *
 * @param[in] temp_c Temperature value in degrees Celsius.
 * @param[in] unit Target temperature unit.
 *
 * @return Converted temperature value.
 */
float BME280_ConvertTemperature(float temp_c,
                                BME280_TempUnit_t unit)
{
    if (unit == BME280_TEMP_UNIT_F)
    {
        return temp_c * 9.0f / 5.0f + 32.0f;
    }

    return temp_c;
}

/**
 * @brief Convert a pressure value from hPa to the selected unit.
 *
 * @param[in] press_hpa Pressure value in hPa.
 * @param[in] unit Target pressure unit.
 *
 * @return Converted pressure value.
 */
float BME280_ConvertPressure(float press_hpa,
                             BME280_PressUnit_t unit)
{
    if (unit == BME280_PRESS_UNIT_MMHG)
    {
        return press_hpa * 0.750062f;
    }

    return press_hpa;
}

/**
 * @brief Convert a humidity value from percent to the selected unit.
 *
 * @param[in] hum_percent Relative humidity in percent.
 * @param[in] unit Target humidity unit.
 *
 * @return Converted humidity value.
 */
float BME280_ConvertHumidity(float hum_percent,
                             BME280_HumUnit_t unit)
{
    if (unit == BME280_HUM_UNIT_FRACTION)
    {
        return hum_percent / 100.0f;
    }

    return hum_percent;
}

/**
 * @brief Read compensated temperature, pressure, and humidity values.
 *
 * The function reads the raw measurement block, applies the Bosch
 * compensation formulas, and stores the resulting physical values into the
 * output structure.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[out] data Pointer to the output data structure.
 *
 * @return 0  Data read and compensation completed successfully.
 * @return -1 Raw data could not be read from the sensor.
 */
int BME280_ReadAll(BME280_Handle_t *handle,
                   BME280_Data_t *data)
{
    int32_t adc_T, adc_P, adc_H;

    if (BME280_ReadRaw(handle, &adc_T, &adc_P, &adc_H) != 0)
    {
        return -1;
    }

    int32_t temperature = BME280_CompensateTemperature(handle, adc_T);
    uint32_t pressure = BME280_CompensatePressure(handle, adc_P);
    uint32_t humidity = BME280_CompensateHumidity(handle, adc_H);

    data->temperature = temperature / 100.0f;
    data->pressure = pressure / 25600.0f;
    data->humidity = humidity / 1024.0f;

    return 0;
}

/**
 * @brief Trigger one forced measurement and read the result.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[out] data Pointer to the output data structure.
 *
 * @return 0  Forced measurement and data read completed successfully.
 * @return -1 Forced measurement could not be started or waited for.
 */
int BME280_ReadForced(BME280_Handle_t *handle,
                      BME280_Data_t *data)
{
    if (BME280_ForceMeasurement(handle) != 0)
    {
        return -1;
    }

    return BME280_ReadAll(handle, data);
}

/**
 * @brief Wait for a normal-mode measurement to complete and read the result.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[out] data Pointer to the output data structure.
 *
 * @return 0  Measurement completed and data were read successfully.
 * @return -1 Measurement did not complete successfully.
 */
int BME280_ReadNormal(BME280_Handle_t *handle,
                      BME280_Data_t *data)
{
    if (BME280_WaitMeasurement(handle) != 0)
    {
        return -1;
    }

    return BME280_ReadAll(handle, data);
}

/**
 * @brief Update the sensor operating mode while preserving the other bits of ctrl_meas.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] mode New mode value.
 *
 * @return 0  Mode updated successfully.
 * @return -1 Reading ctrl_meas failed.
 * @return -2 Writing ctrl_meas failed.
 */
int BME280_SetMode(BME280_Handle_t *handle,
                   uint8_t mode)
{
    uint8_t ctrl_meas;

    /* Read the current register value so only the mode bits are changed. */
    if (BME280_ReadRegister(handle, BME280_REG_CTRL_MEAS, &ctrl_meas, 1) != 0)
    {
        return -1;
    }

    /* Clear mode bits [1:0] before writing the new mode. */
    ctrl_meas &= (uint8_t)~0x03U;

    /* Apply the requested mode, keeping all other fields intact. */
    ctrl_meas |= (mode & 0x03U);

    if (BME280_WriteRegister(handle, BME280_REG_CTRL_MEAS, ctrl_meas) != 0)
    {
        return -2;
    }

    return 0;
}

/**
 * @brief Update the standby time field in the configuration register.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] t_sb Standby time field value.
 *
 * @return 0  Standby time updated successfully.
 * @return -1 Reading config failed.
 * @return -2 Writing config failed.
 */
int BME280_SetStandby(BME280_Handle_t *handle,
                      uint8_t t_sb)
{
    uint8_t config;

    if (BME280_ReadRegister(handle, BME280_REG_CONFIG, &config, 1) != 0)
    {
        return -1;
    }

    /* Clear t_sb bits [7:5] before applying the requested standby period. */
    config &= (uint8_t)~(0x07U << 5);

    /* Store only the valid field bits. */
    config |= (t_sb & (0x07U << 5));

    if (BME280_WriteRegister(handle, BME280_REG_CONFIG, config) != 0)
    {
        return -2;
    }

    return 0;
}

/**
 * @brief Configure oversampling for temperature, pressure, and humidity.
 *
 * The humidity oversampling is written through ctrl_hum, while the temperature
 * and pressure oversampling fields are updated in ctrl_meas.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] osrs_t Temperature oversampling value.
 * @param[in] osrs_p Pressure oversampling value.
 * @param[in] osrs_h Humidity oversampling value.
 *
 * @return  0  Oversampling configuration updated successfully.
 * @return -1 Writing ctrl_hum failed.
 * @return -2 Reading ctrl_meas failed.
 * @return -3 Writing ctrl_meas failed.
 */
int BME280_SetOversampling(BME280_Handle_t *handle,
                           uint8_t osrs_t,
                           uint8_t osrs_p,
                           uint8_t osrs_h)
{
    uint8_t ctrl_meas;
    uint8_t ctrl_hum;

    /* Update humidity oversampling first, as required by the datasheet. */
    ctrl_hum = osrs_h & 0x07U;

    if (BME280_WriteRegister(handle, BME280_REG_CTRL_HUM, ctrl_hum) != 0)
    {
        return -1;
    }

    /* Read the current ctrl_meas value to preserve unrelated bits. */
    if (BME280_ReadRegister(handle, BME280_REG_CTRL_MEAS, &ctrl_meas, 1) != 0)
    {
        return -2;
    }

    /* Clear temperature and pressure oversampling fields. */
    ctrl_meas &= (uint8_t)~((0x07U << 5) | (0x07U << 2));

    /* Apply the requested oversampling settings. */
    ctrl_meas |= (uint8_t)((osrs_t << 5) | (osrs_p << 2));

    if (BME280_WriteRegister(handle, BME280_REG_CTRL_MEAS, ctrl_meas) != 0)
    {
        return -3;
    }

    return 0;
}

/**
 * @brief Update the IIR filter coefficient in the configuration register.
 *
 * @param[in,out] handle Pointer to the sensor handle.
 * @param[in] filter Filter configuration value.
 *
 * @return 0  Filter configuration updated successfully.
 * @return -1 Reading config failed.
 * @return -2 Writing config failed.
 */
int BME280_SetFilter(BME280_Handle_t *handle,
                     uint8_t filter)
{
    uint8_t config;

    if (BME280_ReadRegister(handle, BME280_REG_CONFIG, &config, 1) != 0)
    {
        return -1;
    }

    /* Clear filter bits [4:2] before applying the new coefficient. */
    config &= (uint8_t)~(0x07U << 2);

    /* Store only the valid field bits. */
    config |= (filter & (0x07U << 2));

    if (BME280_WriteRegister(handle, BME280_REG_CONFIG, config) != 0)
    {
        return -2;
    }

    return 0;
}

/**
 * @brief Apply a full sensor configuration in a safe order.
 *
 * The function first stores the current operating mode, then switches the
 * sensor to sleep mode before updating the oversampling, filter, and standby
 * fields. After all configuration registers are updated, the previous mode is
 * restored.
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
int BME280_Configure(BME280_Handle_t *handle,
                     const BME280_Config_t *cfg)
{
    uint8_t prev_mode;

    /* Read the current mode so it can be restored after reconfiguration. */
    if (BME280_ReadRegister(handle, BME280_REG_CTRL_MEAS, &prev_mode, 1) != 0)
    {
        return -1;
    }

    prev_mode &= 0x03U;

    /* Enter sleep mode before modifying configuration registers. */
    if (BME280_SetMode(handle, BME280_MODE_SLEEP) != 0)
    {
        return -2;
    }

    /* Apply oversampling settings for all three measurement channels. */
    if (BME280_SetOversampling(handle,
                               cfg->osrs_t,
                               cfg->osrs_p,
                               cfg->osrs_h) != 0)
    {
        return -3;
    }

    /* Apply digital filtering settings. */
    if (BME280_SetFilter(handle, cfg->filter) != 0)
    {
        return -4;
    }

    /* Apply standby timing settings. */
    if (BME280_SetStandby(handle, cfg->standby) != 0)
    {
        return -5;
    }

    /* Restore the operating mode that was active before reconfiguration. */
    if (BME280_SetMode(handle, prev_mode) != 0)
    {
        return -6;
    }

    return 0;
}
