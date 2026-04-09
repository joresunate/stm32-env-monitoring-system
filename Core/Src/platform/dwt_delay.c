/**
 * @file dwt_delay.c
 * @brief Precise blocking delay implementation using the Cortex-M DWT unit.
 *
 * This module provides cycle-based delay routines suitable for short timing
 * requirements where deterministic blocking delays are acceptable.
 */

#include <platform/dwt_delay.h>

/**
 * @brief Initialize the DWT cycle counter.
 *
 * The function enables the trace unit, resets the cycle counter, and then
 * enables CYCCNT for subsequent delay measurements.
 *
 * @return None.
 *
 * @note The routine enters an infinite loop if the cycle counter cannot be
 *       enabled, because the delay subsystem is considered a critical service.
 */
void DWT_Init(void)
{
    /* Enable trace and debug block access. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Reset the cycle counter before enabling timing measurements. */
    DWT->CYCCNT = 0U;

    /* Enable the cycle counter. */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* Verify that the counter was enabled successfully. */
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U)
    {
        /* Critical failure: timing base is unavailable. */
        while (1)
        {
        }
    }
}

/**
 * @brief Generate a blocking delay in microseconds.
 *
 * The delay is implemented by waiting for the required number of CPU cycles
 * to elapse based on the current SystemCoreClock value.
 *
 * @param[in] us Delay time in microseconds.
 *
 * @return None.
 */
void DWT_DelayUs(uint32_t us)
{
    /* Convert the requested time into CPU cycles. */
    uint64_t cycles = ((uint64_t)SystemCoreClock * us) / 1000000U;

    uint32_t start = DWT->CYCCNT;

    /* Busy-wait until the target number of cycles has elapsed. */
    while ((uint32_t)(DWT->CYCCNT - start) < (uint32_t)cycles)
    {
        /* Busy wait. */
    }
}

/**
 * @brief Generate a blocking delay in milliseconds.
 *
 * The delay is implemented by waiting for the required number of CPU cycles
 * to elapse based on the current SystemCoreClock value.
 *
 * @param[in] ms Delay time in milliseconds.
 *
 * @return None.
 */
void DWT_DelayMs(uint32_t ms)
{
    /* Convert the requested time into CPU cycles. */
    uint64_t cycles = ((uint64_t)SystemCoreClock * ms) / 1000U;

    uint32_t start = DWT->CYCCNT;

    /* Busy-wait until the target number of cycles has elapsed. */
    while ((uint32_t)(DWT->CYCCNT - start) < (uint32_t)cycles)
    {
        /* Busy wait. */
    }
}
