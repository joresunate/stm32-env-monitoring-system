/**
 * @file dwt_delay.h
 * @brief Precise blocking delay interface based on the DWT cycle counter.
 *
 * This header provides initialization and delay routines that use the
 * Cortex-M4 Data Watchpoint and Trace unit for cycle-accurate timing.
 */

#ifndef DWT_DELAY_H
#define DWT_DELAY_H

#include <stdint.h>
#include "stm32f4xx.h"

/**
 * @brief Initialize the DWT cycle counter.
 *
 * @return None.
 *
 * @note This function must be called before any DWT-based delay routine.
 */
void DWT_Init(void);

/**
 * @brief Generate a blocking delay in microseconds.
 *
 * @param[in] us Delay time in microseconds.
 *
 * @return None.
 */
void DWT_DelayUs(uint32_t us);

/**
 * @brief Generate a blocking delay in milliseconds.
 *
 * @param[in] ms Delay time in milliseconds.
 *
 * @return None.
 */
void DWT_DelayMs(uint32_t ms);

#endif
