/**
 * @file led.h
 * @brief LED control interface for the STM32F4DISCOVERY board.
 *
 * This header defines the LED mapping used by the project and exposes a
 * minimal API for initializing the board LEDs, switching them on and off,
 * toggling their state, and controlling all LEDs simultaneously.
 */

#ifndef LED_H
#define LED_H

#include "stm32f4xx.h"

/**
 * @brief LED identifiers mapped to STM32F4DISCOVERY GPIOD pins.
 *
 * The board LEDs are connected to the following pins:
 * - PD12: Green
 * - PD13: Orange
 * - PD14: Red
 * - PD15: Blue
 */
typedef enum
{
    LED_PIN_GREEN  = 12, /**< Green LED on PD12. */
    LED_PIN_ORANGE = 13, /**< Orange LED on PD13. */
    LED_PIN_RED    = 14, /**< Red LED on PD14. */
    LED_PIN_BLUE   = 15  /**< Blue LED on PD15. */

} LED_t;

/**
 * @brief Initialize GPIOD pins used for the board LEDs.
 *
 * The function enables the peripheral clock and configures PD12-PD15 as
 * general-purpose push-pull outputs.
 *
 * @return None.
 */
void LED_Init(void);

/**
 * @brief Turn on the selected LED.
 *
 * @param[in] led LED identifier mapped to the corresponding GPIOD pin.
 *
 * @return None.
 */
void LED_On(LED_t led);

/**
 * @brief Turn off the selected LED.
 *
 * @param[in] led LED identifier mapped to the corresponding GPIOD pin.
 *
 * @return None.
 */
void LED_Off(LED_t led);

/**
 * @brief Toggle the selected LED state.
 *
 * @param[in] led LED identifier mapped to the corresponding GPIOD pin.
 *
 * @return None.
 *
 * @note The implementation uses the BSRR register for atomic set and reset
 *       operations to avoid read-modify-write races.
 */
void LED_Toggle(LED_t led);

/**
 * @brief Turn on all board LEDs.
 *
 * @return None.
 */
void LED_All_On(void);

/**
 * @brief Turn off all board LEDs.
 *
 * @return None.
 */
void LED_All_Off(void);

#endif
