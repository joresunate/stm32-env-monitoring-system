/**
 * @file timer6.c
 * @brief TIM6 configuration for periodic system triggering.
 *
 * This module configures TIM6 to generate a 1 Hz update interrupt used as a
 * periodic trigger source for the environmental measurement cycle.
 */

#include <platform/timer6.h>

/**
 * @brief Initialize TIM6 to generate a 1 Hz update interrupt.
 *
 * The timer clock is derived from APB1. The prescaler and auto-reload values
 * are selected so that the update event occurs once per second.
 *
 * @return None.
 */
void TIM6_Init(void)
{
    /* Enable the TIM6 peripheral clock. */
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    /* Stop the counter before configuring timer registers. */
    TIM6->CR1 &= ~TIM_CR1_CEN;

    /*
     * Configure the prescaler and auto-reload registers.
     *
     * TIM6 clock = 50 MHz (APB1 = 25 MHz, timer clock multiplied by 2).
     * PSC = 49999  -> counter clock = 50 MHz / 50000 = 1 kHz.
     * ARR = 999    -> update event frequency = 1 kHz / 1000 = 1 Hz.
     */
    TIM6->PSC = 49999U;
    TIM6->ARR = 999U;

    /* Enable update interrupt generation. */
    TIM6->DIER |= TIM_DIER_UIE;

    /*
     * Generate an update event to load prescaler and auto-reload values into
     * the active shadow registers and to reset the counter state.
     */
    TIM6->EGR |= TIM_EGR_UG;

    /* Enable TIM6 interrupt in the NVIC. */
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    /* Configure interrupt priority. Lower value means higher priority. */
    NVIC_SetPriority(TIM6_DAC_IRQn, 1U);

    /* Start the timer counter. */
    TIM6->CR1 |= TIM_CR1_CEN;
}
