/**
 * @file system_fsm.h
 * @brief High-level system finite state machine interface.
 *
 * This header exposes initialization and execution routines for the main
 * application state machine that coordinates sensor acquisition, processing,
 * and output handling.
 */

#ifndef SYSTEM_FSM_H
#define SYSTEM_FSM_H

#include <system/system_context.h>

/**
 * @brief Default maximum number of error recovery attempts.
 */
#define SYSTEM_ERROR_LIMIT_DEFAULT  3U

/**
 * @brief Buffer size used for UART text formatting in the FSM output stage.
 */
#define SYSTEM_UART_BUFFER_SIZE     96U

/**
 * @brief Initialize the system finite state machine.
 *
 * The function sets the initial state, default environmental classification,
 * error counters, and clears cached measurement values.
 *
 * @return None.
 */
void System_FSM_Init(void);

/**
 * @brief Execute one iteration of the system finite state machine.
 *
 * The function must be called repeatedly from the main application loop.
 * It advances the system through initialization, measurement, processing,
 * output, and error handling states.
 *
 * @return None.
 */
void System_FSM_Run(void);

#endif
