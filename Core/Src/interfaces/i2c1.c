/**
 * @file i2c1.c
 * @brief Bare-metal register-level I2C1 master driver.
 *
 * This module configures I2C1 and implements blocking master transactions
 * required by the LCD expander and sensor communication layers.
 */

#include <interfaces/i2c1.h>

/**
 * @brief Initialize I2C1 peripheral and associated GPIO pins.
 *
 * PB8 is configured as SCL and PB9 as SDA in alternate function 4 mode.
 * The peripheral is then reset and configured for standard-mode operation
 * at 100 kHz.
 *
 * @return None.
 */
void I2C1_Init(void)
{
    /* Enable clocks for I2C1 and GPIOB. */
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    /* Configure PB8 (SCL) and PB9 (SDA) as alternate function pins. */
    GPIOB->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9);
    GPIOB->MODER |= (2U << GPIO_MODER_MODE8_Pos) |
                    (2U << GPIO_MODER_MODE9_Pos);

    /* Use open-drain output type as required by the I2C bus. */
    GPIOB->OTYPER |= GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9;

    /* Select high-speed output for cleaner I2C edges. */
    GPIOB->OSPEEDR |= (3U << GPIO_OSPEEDR_OSPEED8_Pos) |
                      (3U << GPIO_OSPEEDR_OSPEED9_Pos);

    /* Enable pull-up resistors on SCL and SDA lines. */
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD8 | GPIO_PUPDR_PUPD9);
    GPIOB->PUPDR |= (1U << GPIO_PUPDR_PUPD8_Pos) |
                    (1U << GPIO_PUPDR_PUPD9_Pos);

    /* Select AF4 for both pins to route them to I2C1. */
    GPIOB->AFR[1] &= ~((0xFU << 0) | (0xFU << 4));
    GPIOB->AFR[1] |= ((4U << 0) | (4U << 4));

    /* Reset the peripheral to start from a known configuration state. */
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    /* APB1 clock frequency is 25 MHz in this project configuration. */
    I2C1->CR2 = (I2C1->CR2 & ~I2C_CR2_FREQ) | 25U;

    /* Standard mode timing for 100 kHz operation. */
    I2C1->CCR = 125U;

    /* Rise-time configuration for the selected peripheral clock. */
    I2C1->TRISE = 26U;

    /* Enable the peripheral after all timing parameters are configured. */
    I2C1->CR1 |= I2C_CR1_PE;
}

/**
 * @brief Generate a START condition and send the slave address.
 *
 * The function waits for the bus to become free, requests START, waits for
 * the SB flag, sends the address byte, and checks for address acknowledgment.
 *
 * @param[in] address 7-bit I2C slave address.
 * @param[in] direction Transfer direction.
 *                      0 = write, 1 = read.
 *
 * @return 0  START and address phase completed successfully.
 * @return -1 Bus remained busy until timeout expired.
 * @return -2 START flag was not set before timeout expired.
 * @return -3 ADDR or AF flag did not appear before timeout expired.
 * @return -4 Address phase was NACKed by the slave.
 */
int I2C1_Start(uint8_t address, uint8_t direction)
{
    uint32_t timeout = I2C1_TIMEOUT_COUNTER;

    /* Wait until the bus becomes free. */
    while (I2C1->SR2 & I2C_SR2_BUSY)
    {
        if (--timeout == 0U)
        {
            return -1;
        }
    }

    /* Request a START condition. */
    I2C1->CR1 |= I2C_CR1_START;

    /* Wait for the START bit flag. */
    timeout = I2C1_TIMEOUT_COUNTER;
    while ((I2C1->SR1 & I2C_SR1_SB) == 0U)
    {
        if (--timeout == 0U)
        {
            return -2;
        }
    }

    /* Transmit the 7-bit address and the direction bit. */
    I2C1->DR = (uint8_t)((address << 1) | (direction & 0x01U));

    /* Wait for address acknowledgment or NACK detection. */
    timeout = I2C1_TIMEOUT_COUNTER;
    while ((I2C1->SR1 & (I2C_SR1_ADDR | I2C_SR1_AF)) == 0U)
    {
        if (--timeout == 0U)
        {
            return -3;
        }
    }

    /* Handle NACK on address phase. */
    if ((I2C1->SR1 & I2C_SR1_AF) != 0U)
    {
        I2C1->SR1 &= ~I2C_SR1_AF;
        I2C1_Stop();
        return -4;
    }

    /* Clear the ADDR flag by reading SR1 followed by SR2. */
    volatile uint32_t temp_reg;
    temp_reg = I2C1->SR1;
    temp_reg = I2C1->SR2;
    (void)temp_reg;

    return 0;
}

/**
 * @brief Write one byte over I2C.
 *
 * The function waits for the transmit buffer to become empty, writes the data
 * byte, and then waits for the byte transfer to complete.
 *
 * @param[in] data Data byte to transmit.
 *
 * @return 0  Byte transmitted successfully.
 * @return -1 TXE flag did not become ready before timeout expired.
 * @return -2 BTF flag did not become set before timeout expired.
 */
int I2C1_Write(uint8_t data)
{
    uint32_t timeout = I2C1_TIMEOUT_COUNTER;

    /* Wait until the transmit buffer becomes empty. */
    while ((I2C1->SR1 & I2C_SR1_TXE) == 0U)
    {
        if (--timeout == 0U)
        {
            return -1;
        }
    }

    /* Place the next byte into the data register. */
    I2C1->DR = data;

    /* Wait until the byte transfer is fully completed. */
    timeout = I2C1_TIMEOUT_COUNTER;
    while ((I2C1->SR1 & I2C_SR1_BTF) == 0U)
    {
        if (--timeout == 0U)
        {
            return -2;
        }
    }

    return 0;
}

/**
 * @brief Generate an I2C STOP condition.
 *
 * @return None.
 */
void I2C1_Stop(void)
{
    /* Request STOP on the bus. */
    I2C1->CR1 |= I2C_CR1_STOP;
}
