/**
 * @file timer6.h
 * @brief TIM6 initialization interface for periodic trigger generation.
 *
 * This header exposes the timer setup routine used by the project to generate
 * a periodic interrupt source that drives sensor measurement requests.
 */

#ifndef TIMER6_H
#define TIMER6_H

#include "stm32f4xx.h"

/**
 * @brief Initialize TIM6 to generate a periodic update interrupt.
 *
 * @return None.
 */
void TIM6_Init(void);

#endif
