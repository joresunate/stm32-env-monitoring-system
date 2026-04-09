/**
 * @file led.c
 * @brief GPIO-based LED control implementation for the STM32F4DISCOVERY board.
 *
 * This module configures the board LEDs on GPIOD and provides atomic control
 * routines for individual LEDs and for the full LED group.
 */

#include <platform/led.h>

/**
 * @brief Initialize the LED GPIO pins.
 *
 * The function enables the GPIOD clock and configures PD12-PD15 as output
 * pins in general-purpose output mode.
 *
 * @return None.
 */
void LED_Init(void)
{
    /* Enable the GPIOD peripheral clock. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    /* Clear mode bits for PD12-PD15. */
    GPIOD->MODER &= ~(0xFFU << 24);

    /* Select general-purpose output mode for PD12-PD15. */
    GPIOD->MODER |= (0x55U << 24);
}

/**
 * @brief Turn on the selected LED.
 *
 * The BSRR lower half is used for atomic pin set operations.
 *
 * @param[in] led LED identifier mapped to the corresponding GPIOD pin.
 *
 * @return None.
 */
void LED_On(LED_t led)
{
    GPIOD->BSRR = (1U << led);
}

/**
 * @brief Turn off the selected LED.
 *
 * The BSRR upper half is used for atomic pin reset operations.
 *
 * @param[in] led LED identifier mapped to the corresponding GPIOD pin.
 *
 * @return None.
 */
void LED_Off(LED_t led)
{
    GPIOD->BSRR = (1U << (led + 16U));
}

/**
 * @brief Toggle the selected LED state.
 *
 * The function reads the current output state from ODR and then performs an
 * atomic set or reset operation through BSRR.
 *
 * @param[in] led LED identifier mapped to the corresponding GPIOD pin.
 *
 * @return None.
 */
void LED_Toggle(LED_t led)
{
    /* Check the current output state before selecting the next action. */
    if (GPIOD->ODR & (1U << led))
    {
        /* LED is currently on, so reset the output pin. */
        GPIOD->BSRR = (1U << (led + 16U));
    }
    else
    {
        /* LED is currently off, so set the output pin. */
        GPIOD->BSRR = (1U << led);
    }
}

/**
 * @brief Turn on all board LEDs.
 *
 * @return None.
 */
void LED_All_On(void)
{
    GPIOD->BSRR = (0xFU << 12);
}

/**
 * @brief Turn off all board LEDs.
 *
 * @return None.
 */
void LED_All_Off(void)
{
    GPIOD->BSRR = (0xFU << (12U + 16U));
}
