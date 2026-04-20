/**
 * @file system_context.h
 * @brief Shared runtime context and state definitions for the monitoring system.
 *
 * This header defines the main system state machine states, the environmental
 * classification states, and the global context structure used by the project
 * runtime logic.
 */

#ifndef SYSTEM_CONTEXT_H
#define SYSTEM_CONTEXT_H

#include <devices/bme280/bme280.h>
#include <devices/lcd/lcd_hd44780_i2c.h>
#include <stdint.h>

/**
 * @brief Main system finite state machine states.
 */
typedef enum
{
    SYSTEM_STATE_INIT = 0,   /**< Perform peripheral and sensor initialization. */
    SYSTEM_STATE_IDLE,       /**< Wait for the next measurement trigger. */
    SYSTEM_STATE_MEASURE,    /**< Acquire sensor data from the BME280. */
    SYSTEM_STATE_PROCESS,    /**< Validate and classify measured data. */
    SYSTEM_STATE_OUTPUT,     /**< Update LCD, UART, and LED outputs. */
    SYSTEM_STATE_ERROR       /**< Handle runtime or sensor failures. */

} SystemState_t;

/**
 * @brief Environmental classification states.
 */
typedef enum
{
    ENV_STATE_UNKNOWN = 0,   /**< No valid classification is available yet. */
    ENV_STATE_NORMAL,        /**< Measured parameters are within normal range. */
    ENV_STATE_WARNING,       /**< Only part of the parameters is outside normal range. */
    ENV_STATE_CRITICAL      /**< Multiple parameters indicate an abnormal condition. */

} EnvState_t;

/**
 * @brief Main runtime context of the monitoring system.
 *
 * This structure stores the current FSM state, environmental classification,
 * latest sensor data, error counters, and driver handles used by the project.
 */
typedef struct
{
    SystemState_t state;      /**< Current system FSM state. */
    EnvState_t env;           /**< Current environmental classification state. */

    BME280_Data_t data;       /**< Latest compensated sensor data. */

    uint8_t error_count;      /**< Number of consecutive errors detected. */
    uint8_t error_limit;      /**< Maximum number of recovery attempts. */

    LCD_Handle_t lcd;         /**< LCD driver handle. */
    BME280_Handle_t bme;      /**< BME280 sensor handle. */

} SystemContext_t;

/**
 * @brief Global system context instance.
 *
 * This object is shared by the FSM and other modules that need access to the
 * current runtime state of the monitoring system.
 */
extern SystemContext_t g_system_context;

#endif
