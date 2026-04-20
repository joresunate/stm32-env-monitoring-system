/**
 * @file system_fsm.c
 * @brief Implementation of the main system finite state machine.
 *
 * This module coordinates LCD initialization, BME280 acquisition, data
 * validation and classification, UART reporting, LED indication, and runtime
 * error handling for the environmental monitoring system.
 */

#include <interfaces/usart2_tx.h>
#include <platform/dwt_delay.h>
#include <platform/led.h>
#include <stdio.h>
#include <system/system_fsm.h>

extern I2C_HandleTypeDef hi2c3;
extern volatile uint8_t g_sensor_trigger_flag;

static uint32_t measurement_counter = 0U;
static uint8_t csv_header_sent = 0U;
static EnvState_t prev_env_state = ENV_STATE_UNKNOWN;

/**
 * @brief Global system context instance.
 */
SystemContext_t g_system_context;

/**
 * @brief Initialize the system finite state machine.
 *
 * The function sets the initial FSM state, clears the environmental
 * classification, resets the error counters, and clears the cached sensor
 * measurement values.
 *
 * @return None.
 */
void System_FSM_Init(void)
{
    g_system_context.state = SYSTEM_STATE_INIT;
    g_system_context.env = ENV_STATE_UNKNOWN;

    g_system_context.error_count = 0U;
    g_system_context.error_limit = SYSTEM_ERROR_LIMIT_DEFAULT;

    /* Clear cached measurement data before the first sensor acquisition. */
    g_system_context.data.temperature = 0.0f;
    g_system_context.data.pressure = 0.0f;
    g_system_context.data.humidity = 0.0f;
}

/**
 * @brief Execute one step of the main system state machine.
 *
 * The FSM handles initialization, waiting for a trigger, sensor acquisition,
 * data validation and classification, output formatting, and error recovery.
 *
 * @return None.
 */
void System_FSM_Run(void)
{
    switch (g_system_context.state)
    {
        case SYSTEM_STATE_INIT:
        {
            /* Initialize the LCD and switch it into the ready state. */
            LCD_InitHandle(&g_system_context.lcd, LCD_DEFAULT_I2C_ADDR);
            LCD_Init(&g_system_context.lcd);

            /* Bind the BME280 driver to the configured I2C peripheral. */
            g_system_context.bme.i2c = &hi2c3;
            g_system_context.bme.address = BME280_I2C_ADDR_LOW;

            /* Initialize the sensor and load calibration/configuration data. */
            if (BME280_Init(&g_system_context.bme) != 0)
            {
                USART2_WriteString("Sensor initialization failed\r\n");
                g_system_context.state = SYSTEM_STATE_ERROR;
            }
            else
            {
                USART2_WriteString("System initialization completed\r\n");
                USART2_WriteString("Environmental monitoring started\r\n");
                g_system_context.state = SYSTEM_STATE_IDLE;
            }

            break;
        }

        case SYSTEM_STATE_IDLE:
        {
            /* Wait until the periodic trigger requests a new measurement cycle. */
            if (g_sensor_trigger_flag != 0U)
            {
                g_sensor_trigger_flag = 0U;
                g_system_context.state = SYSTEM_STATE_MEASURE;
            }
            break;
        }

        case SYSTEM_STATE_MEASURE:
        {
            /* Acquire one forced measurement from the BME280 sensor. */
            if (BME280_ReadForced(&g_system_context.bme,
            					  &g_system_context.data) != 0)
            {
                USART2_WriteString("Sensor read operation failed\r\n");
                g_system_context.state = SYSTEM_STATE_ERROR;
            }
            else
            {
                g_system_context.state = SYSTEM_STATE_PROCESS;
            }
            break;
        }

        case SYSTEM_STATE_PROCESS:
        {
            float temperature = g_system_context.data.temperature;
            float humidity = g_system_context.data.humidity;
            float pressure = g_system_context.data.pressure;

            /* Reject values that are outside the expected physical range. */
            if ((temperature < -40.0f || temperature > 80.0f) ||
                (humidity < 0.0f || humidity > 100.0f) ||
                (pressure < 300.0f || pressure > 1100.0f))
            {
                USART2_WriteString("Measured data out of valid range\r\n");
                g_system_context.state = SYSTEM_STATE_ERROR;
                break;
            }

            /* Determine whether the environment is normal, warning, or critical. */
            uint8_t is_temp_normal = (temperature >= 19.0f && temperature <= 23.0f);
            uint8_t is_hum_normal = (humidity >= 30.0f && humidity <= 60.0f);

            if (is_temp_normal && is_hum_normal)
            {
                g_system_context.env = ENV_STATE_NORMAL;
            }
            else if ((!is_temp_normal) && (!is_hum_normal))
            {
                g_system_context.env = ENV_STATE_CRITICAL;
            }
            else
            {
                g_system_context.env = ENV_STATE_WARNING;
            }

            /* Convert pressure to mmHg for output formatting. */
            g_system_context.data.pressure =
                BME280_ConvertPressure(g_system_context.data.pressure, BME280_PRESS_UNIT_MMHG);

            g_system_context.state = SYSTEM_STATE_OUTPUT;
            break;
        }

        case SYSTEM_STATE_OUTPUT:
        {
            char uart_buf[SYSTEM_UART_BUFFER_SIZE];

            float temperature = g_system_context.data.temperature;
            float humidity = g_system_context.data.humidity;
            float pressure = g_system_context.data.pressure;

            const char *env_str = "UNKNOWN";
            const char *env_str_short = "UNKN";
            LED_t led_to_set = LED_PIN_BLUE;

            /* Select the human-readable status label and the corresponding LED. */
            switch (g_system_context.env)
            {
                case ENV_STATE_NORMAL:
                {
                    env_str = "NORMAL";
                    env_str_short = "NORM";
                    led_to_set = LED_PIN_GREEN;
                    break;
                }

                case ENV_STATE_WARNING:
                {
                    env_str = "WARNING";
                    env_str_short = "WARN";
                    led_to_set = LED_PIN_ORANGE;
                    break;
                }

                case ENV_STATE_CRITICAL:
                {
                    env_str = "CRITICAL";
                    env_str_short = "CRIT";
                    led_to_set = LED_PIN_RED;
                    break;
                }

                default:
                    break;
            }

            /* Update the board LEDs only when the environmental state changes. */
            if (g_system_context.env != prev_env_state)
            {
                LED_All_Off();
                LED_On(led_to_set);
                prev_env_state = g_system_context.env;
            }

            /* Prepare two fixed-width LCD lines for a clean overwrite refresh. */
            char line1[17];
            char line2[17];

            /* Format the first line: temperature and humidity. */
            snprintf(line1, sizeof(line1),
                     "T:%5.1fC  H:%3.0f%%",
                     temperature, humidity);

            /* Format the second line: pressure and short status label. */
            snprintf(line2, sizeof(line2),
                     "P:%4.0fmmHg  %4s",
                     pressure, env_str_short);

            /* Write both rows to the LCD starting from column 0. */
            LCD_SetCursor(&g_system_context.lcd, 0U, 0U);
            LCD_Print(&g_system_context.lcd, line1);

            LCD_SetCursor(&g_system_context.lcd, 1U, 0U);
            LCD_Print(&g_system_context.lcd, line2);

            /* Transmit the CSV header once after startup. */
            if (csv_header_sent == 0U)
            {
                USART2_WriteString("Measurement,Temperature_C,Pressure_mmHg,Humidity_percent,Status\r\n");
                csv_header_sent = 1U;
            }

            measurement_counter++;

            /* Build one CSV line for the PC logging application. */
            snprintf(uart_buf, sizeof(uart_buf),
                     "%lu,%.2f,%.2f,%.2f,%s\r\n",
                     measurement_counter,
                     temperature,
                     pressure,
                     humidity,
                     env_str);

            USART2_WriteString(uart_buf);

            /* Reset error history after a successful output cycle. */
            g_system_context.error_count = 0U;
            g_system_context.state = SYSTEM_STATE_IDLE;

            break;
        }

        case SYSTEM_STATE_ERROR:
        {
            USART2_WriteString("System error detected\r\n");

            g_system_context.error_count++;

            /* Pause before attempting recovery or final halt. */
            DWT_DelayMs(1000);

            if (g_system_context.error_count < g_system_context.error_limit)
            {
                /* Retry the full initialization sequence. */
                g_system_context.state = SYSTEM_STATE_INIT;
            }
            else
            {
                USART2_WriteString("Fatal error: system halted\r\n");

                /* Clear the LCD and show a final error message. */
                LCD_Clear(&g_system_context.lcd);
                LCD_Print(&g_system_context.lcd, "Fatal error");

                /* Turning off all LEDs */
                LED_All_Off();

                /* Indicate a permanent failure by blinking the red LED forever. */
                while (1)
                {
                    LED_Toggle(LED_PIN_RED);
                    DWT_DelayMs(250U);
                }
            }
            break;
        }

        default:
        {
            /* Recover from an unexpected state by restarting the FSM. */
            g_system_context.state = SYSTEM_STATE_INIT;
            break;
        }
    }
}
