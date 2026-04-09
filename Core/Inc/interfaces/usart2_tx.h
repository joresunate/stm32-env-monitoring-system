/**
 * @file usart2_tx.h
 * @brief Minimal USART2 transmit-only interface for diagnostic output.
 *
 * This header provides a compact API for initializing USART2 and sending
 * characters or strings to the connected USB-UART bridge.
 */

#ifndef USART2_TX_H
#define USART2_TX_H

/**
 * @brief Initialize USART2 in transmit-only mode.
 *
 * @return None.
 */
void USART2_Init(void);

/**
 * @brief Transmit one character over USART2.
 *
 * @param[in] c Character to transmit.
 *
 * @return None.
 */
void USART2_WriteChar(char c);

/**
 * @brief Transmit a null-terminated string over USART2.
 *
 * @param[in] str Pointer to the null-terminated string.
 *
 * @return None.
 */
void USART2_WriteString(const char *str);

#endif
