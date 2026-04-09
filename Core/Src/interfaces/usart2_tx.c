/**
 * @file usart2_tx.c
 * @brief Bare-metal USART2 transmit-only driver implementation.
 *
 * This module configures USART2 for blocking text output used by the project
 * to send diagnostic messages and CSV measurement data to the PC.
 */

#include <interfaces/usart2_tx.h>
#include "stm32f4xx.h"

/**
 * @brief Initialize USART2 for transmit-only operation on PA2.
 *
 * The function enables the required peripheral clocks, configures PA2 as an
 * alternate-function pin mapped to USART2_TX, and sets the baud rate to
 * 115200 bps.
 *
 * @return None.
 */
void USART2_Init(void)
{
    /* Enable clocks for GPIOA and USART2. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* Configure PA2 as alternate function mode. */
    GPIOA->MODER &= ~(GPIO_MODER_MODE2);
    GPIOA->MODER |= (2U << GPIO_MODER_MODE2_Pos);

    /* Select AF7 for USART2_TX on PA2. */
    GPIOA->AFR[0] &= ~(0xFU << (4U * 2U));
    GPIOA->AFR[0] |= (7U << (4U * 2U));

    /* Configure the TX pin for push-pull output with high speed. */
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT2;
    GPIOA->OSPEEDR |= (3U << GPIO_OSPEEDR_OSPEED2_Pos);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD2);

    /* Disable USART2 before changing its configuration. */
    USART2->CR1 = 0U;

    /* Set baud rate to 115200 bps for APB1 clock = 25 MHz. */
    USART2->BRR = (13U << 4) | 9U;

    /* Enable transmitter only; reception is not used in this project. */
    USART2->CR1 |= USART_CR1_TE;

    /* Enable USART2 peripheral. */
    USART2->CR1 |= USART_CR1_UE;
}

/**
 * @brief Transmit one character over USART2.
 *
 * The function blocks until the transmit data register becomes empty and then
 * writes the character to the data register.
 *
 * @param[in] c Character to transmit.
 *
 * @return None.
 */
void USART2_WriteChar(char c)
{
    while ((USART2->SR & USART_SR_TXE) == 0U)
    {
        /* Wait until the transmit data register becomes empty. */
    }

    USART2->DR = (uint16_t)c;
}

/**
 * @brief Transmit a null-terminated string over USART2.
 *
 * @param[in] str Pointer to the null-terminated string.
 *
 * @return None.
 */
void USART2_WriteString(const char *str)
{
    while (*str != '\0')
    {
        USART2_WriteChar(*str++);
    }
}
