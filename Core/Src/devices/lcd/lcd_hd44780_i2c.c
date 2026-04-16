/**
 * @file lcd_hd44780_i2c.c
 * @brief Implementation of the HD44780 LCD driver via a PCF8574 I2C expander.
 *
 * This module provides low-level bus access and high-level helper routines for
 * driving a character LCD in 4-bit mode through an external I/O expander.
 * Timing-sensitive operations use the DWT-based delay services.
 */

#include <devices/lcd/lcd_hd44780_i2c.h>
#include <interfaces/i2c1.h>
#include <platform/dwt_delay.h>
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* Low-level PCF8574 access                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Write one byte to the PCF8574 expander over I2C.
 *
 * The function performs a complete blocking I2C transaction:
 * start condition, address phase, data phase, and stop condition.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] data Byte to write to the PCF8574 output port.
 *
 * @return 0  Operation completed successfully.
 * @return -1 I2C start condition or address transmission failed.
 * @return -2 I2C data transmission failed.
 */
static int LCD_WriteExpander(LCD_Handle_t *lcd,
						 	 uint8_t data)
{
    if (I2C1_Start(lcd->address, 0) != 0)
    {
        return -1;
    }

    if (I2C1_Write(data) != 0)
    {
        return -2;
    }

    I2C1_Stop();

    return 0;
}


/**
 * @brief Write the cached port state to the PCF8574.
 *
 * The function transmits the current value stored in lcd->port_state,
 * which reflects both control signals and data lines.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return 0  Operation completed successfully.
 * @return -1 I2C start condition or address transmission failed.
 * @return -2 I2C data transmission failed.
 */
static int LCD_WritePort(LCD_Handle_t *lcd)
{
    return LCD_WriteExpander(lcd, lcd->port_state);
}


/**
 * @brief Generate an enable pulse for the LCD controller.
 *
 * The HD44780 latches data on the falling edge of EN. This helper toggles
 * the signal high and then low with the required pulse width.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 *
 * @note This function is blocking and uses a short DWT-based delay to satisfy
 *       the minimum enable pulse width and post-strobe execution time.
 */
static void LCD_PulseEnable(LCD_Handle_t *lcd)
{
	/* Raise EN to prepare the latch cycle. */
    lcd->port_state |= LCD_EN;
    LCD_WritePort(lcd);

    /* Hold the pulse long enough for the controller to sample the bus. */
    DWT_DelayUs(1);

    /* Lower EN to complete the latch cycle. */
    lcd->port_state &= ~LCD_EN;
    LCD_WritePort(lcd);

    /* Allow the controller to finish the current instruction. */
    DWT_DelayUs(LCD_CMD_DELAY_US);
}

/**
 * @brief Map a 4-bit nibble to the LCD data pins D4-D7.
 *
 * The function updates only the data bits in the cached port state.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] nibble Lower 4 bits contain the value to send.
 *
 * @return None.
 */
static void LCD_SetDataPins(LCD_Handle_t *lcd,
							uint8_t nibble)
{
    /* Clear the previous data pattern before applying the new nibble. */
    lcd->port_state &= ~(LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7);

    /* Map bit 0 to D4. */
    if (nibble & 0x01U)
    {
        lcd->port_state |= LCD_D4;
    }

    /* Map bit 1 to D5. */
    if (nibble & 0x02U)
    {
        lcd->port_state |= LCD_D5;
    }

    /* Map bit 2 to D6. */
    if (nibble & 0x04U)
    {
        lcd->port_state |= LCD_D6;
    }

    /* Map bit 3 to D7. */
    if (nibble & 0x08U)
    {
        lcd->port_state |= LCD_D7;
    }
}

/**
 * @brief Write one 4-bit nibble to the LCD.
 *
 * The function configures RS and RW, places the nibble on the bus,
 * writes the expander state, and generates the enable pulse.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] nibble 4-bit value to send.
 * @param[in] mode Transfer mode selector: LCD_CMD or LCD_DATA.
 *
 * @return None.
 */
static void LCD_WriteNibble(LCD_Handle_t *lcd,
							uint8_t nibble,
							uint8_t mode)
{
    /* Select the target register. */
    if (mode == LCD_DATA)
    {
        lcd->port_state |= LCD_RS;
    }
    else
    {
        lcd->port_state &= ~LCD_RS;
    }

    /* The driver uses write-only transfers, so RW remains low. */
    lcd->port_state &= ~LCD_RW;

    /* Update the data lines with the selected nibble. */
    LCD_SetDataPins(lcd, nibble);

    /* Push the current bus state to the expander. */
    LCD_WritePort(lcd);

    /* Strobe EN to latch the nibble into the LCD controller. */
    LCD_PulseEnable(lcd);
}

/**
 * @brief Write a full byte to the LCD as two 4-bit transfers.
 *
 * The high nibble is transmitted first, followed by the low nibble.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] byte Byte to send.
 * @param[in] mode Transfer mode selector: LCD_CMD or LCD_DATA.
 *
 * @return None.
 */
static void LCD_WriteByte(LCD_Handle_t *lcd,
						  uint8_t byte,
						  uint8_t mode)
{
    LCD_WriteNibble(lcd, (uint8_t)(byte >> 4), mode);
    LCD_WriteNibble(lcd, (uint8_t)(byte & 0x0FU), mode);
}

/**
 * @brief Write a raw nibble during the LCD initialization sequence.
 *
 * This helper is used before the controller is fully switched to 4-bit mode.
 * RS and RW are forced low because only command-like initialization pulses
 * are required at this stage.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] nibble 4-bit value to send.
 *
 * @return None.
 */
static void LCD_WriteNibbleRaw(LCD_Handle_t *lcd,
							   uint8_t nibble)
{
    /* Force command mode during the power-up handshake. */
    lcd->port_state &= ~LCD_RS;
    lcd->port_state &= ~LCD_RW;

    /* Place the raw nibble on the bus and strobe the controller. */
    LCD_SetDataPins(lcd, nibble);
    LCD_WritePort(lcd);
    LCD_PulseEnable(lcd);
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the LCD descriptor structure.
 *
 * The function stores the I2C address of the PCF8574 and sets the initial
 * cached port state to enable the backlight.
 *
 * @param[out] lcd Pointer to the LCD descriptor structure.
 * @param[in] address I2C address of the PCF8574 expander.
 *
 * @return None.
 */
void LCD_InitHandle(LCD_Handle_t *lcd,
					uint8_t address)
{
    lcd->address = address;

    /* Keep the backlight enabled by default after initialization. */
    lcd->port_state = LCD_BL;
}

/**
 * @brief Perform the HD44780 initialization sequence in 4-bit mode.
 *
 * The sequence follows the datasheet requirements for switching the controller
 * from its power-up state into a stable 4-bit operation mode.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 *
 * @note This function is blocking and must be called only after the DWT
 *       delay service has been initialized.
 */
void LCD_Init(LCD_Handle_t *lcd)
{
    /* Allow the LCD controller to finish internal power-up reset. */
    DWT_DelayMs(50);

    /* Send the standard 8-bit reset sequence using raw nibbles. */
    LCD_WriteNibbleRaw(lcd, 0x03U);
    DWT_DelayMs(5);

    LCD_WriteNibbleRaw(lcd, 0x03U);
    DWT_DelayUs(150);

    LCD_WriteNibbleRaw(lcd, 0x03U);
    DWT_DelayUs(150);

    /* Switch the controller into 4-bit mode. */
    LCD_WriteNibbleRaw(lcd, 0x02U);
    DWT_DelayUs(150);

    /* Configure the function set: 4-bit bus, two lines, 5x8 font. */
    LCD_WriteCommand(lcd,
        LCD_FUNCTION_SET |
        LCD_4BIT_MODE |
        LCD_2LINE |
        LCD_5x8_DOTS
    );

    /* Temporarily turn the display off during setup. */
    LCD_WriteCommand(lcd, LCD_DISPLAY_CTRL);

    /* Clear visible content and return the cursor to home position. */
    LCD_WriteCommand(lcd, LCD_CLEAR_DISPLAY);
    DWT_DelayMs(2);

    /* Select automatic cursor increment without display shift. */
    LCD_WriteCommand(lcd,
        LCD_ENTRY_MODE_SET |
        LCD_ENTRY_INC |
        LCD_ENTRY_SHIFT_OFF
    );

    /* Enable the display with cursor and blink disabled. */
    LCD_WriteCommand(lcd,
        LCD_DISPLAY_CTRL |
        LCD_DISPLAY_ON |
        LCD_CURSOR_OFF |
        LCD_BLINK_OFF
    );
}

/**
 * @brief Send a command byte to the LCD controller.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] command Command byte to transmit.
 *
 * @return None.
 */
void LCD_WriteCommand(LCD_Handle_t *lcd,
				   	  uint8_t command)
{
    LCD_WriteByte(lcd, command, LCD_CMD);
}

/**
 * @brief Send a data byte to the LCD controller.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] data Data byte to transmit.
 *
 * @return None.
 */
void LCD_WriteData(LCD_Handle_t *lcd,
				   uint8_t data)
{
    LCD_WriteByte(lcd, data, LCD_DATA);
}

/**
 * @brief Clear the display contents.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 *
 * @note The clear command requires a longer execution time than standard
 *       write operations, therefore the function inserts an additional delay.
 */
void LCD_Clear(LCD_Handle_t *lcd)
{
    LCD_WriteCommand(lcd, LCD_CLEAR_DISPLAY);
    DWT_DelayMs(2);
}

/**
 * @brief Return the cursor to the home position.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 *
 * @note The home command requires a longer execution time than standard
 *       write operations, therefore the function inserts an additional delay.
 */
void LCD_Home(LCD_Handle_t *lcd)
{
    LCD_WriteCommand(lcd, LCD_RETURN_HOME);
    DWT_DelayMs(2);
}

/**
 * @brief Set the cursor position using row and column coordinates.
 *
 * The function converts the logical row and column coordinates into a DDRAM
 * address according to the standard 16x2 LCD memory map.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] row Target row index. Valid values are 0 and 1.
 * @param[in] col Target column index.
 *
 * @return None.
 *
 * @note If the row index is out of range, the function exits without sending
 *       any command. The column index is forwarded to the DDRAM mapping as
 *       permitted by the controller.
 */
void LCD_SetCursor(LCD_Handle_t *lcd,
				   uint8_t row,
				   uint8_t col)
{
    if (row >= LCD_ROWS)
    {
        return;
    }

    /* Row 0 starts at DDRAM address 0x00, row 1 starts at 0x40. */
    uint8_t addr = (row == 0U) ? col : (0x40U + col);

    LCD_WriteCommand(lcd, (uint8_t)(LCD_SET_DDRAM | addr));
}

/**
 * @brief Set the DDRAM address directly.
 *
 * The function accepts only the visible address ranges typically used by a
 * 16x2 character LCD.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] addr DDRAM address to select.
 *
 * @return None.
 *
 * @note Invalid addresses are ignored silently.
 */
void LCD_SetDDRAM(LCD_Handle_t *lcd,
				  uint8_t addr)
{
    if ((addr <= 0x27U) || ((addr >= 0x40U) && (addr <= 0x67U)))
    {
        LCD_WriteCommand(lcd, (uint8_t)(LCD_SET_DDRAM | addr));
    }
}

/**
 * @brief Write a single character at the current cursor position.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] c Character to write.
 *
 * @return None.
 */
void LCD_PutChar(LCD_Handle_t *lcd,
			  	 char c)
{
    LCD_WriteData(lcd, (uint8_t)c);
}

/**
 * @brief Write a null-terminated string to the LCD.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] str Pointer to the null-terminated string.
 *
 * @return None.
 */
void LCD_Print(LCD_Handle_t *lcd,
			   const char *str)
{
    while (*str != '\0')
    {
        LCD_PutChar(lcd, *str++);
    }
}

/**
 * @brief Write formatted text to the LCD.
 *
 * The function formats the message into a local buffer and then sends it to
 * the display as a null-terminated string.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] fmt Format string in printf-style syntax.
 *
 * @return None.
 *
 * @note The formatted output is limited by LCD_PRINTF_BUFFER_SIZE. If the
 *       generated string does not fit into the internal buffer, it is
 *       truncated and force-terminated with a null character.
 */
void LCD_Printf(LCD_Handle_t *lcd, const char *fmt, ...)
{
    char buffer[LCD_PRINTF_BUFFER_SIZE];

    va_list args;
    va_start(args, fmt);

    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);

    /* Guarantee string termination even if formatting overflows the buffer. */
    if ((len < 0) || (len >= (int)sizeof(buffer)))
    {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    LCD_Print(lcd, buffer);
}

/**
 * @brief Set the LCD entry mode.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] inc Increment/decrement mode flag.
 * @param[in] shift Display shift enable/disable flag.
 *
 * @return None.
 */
void LCD_SetEntryMode(LCD_Handle_t *lcd,
					  uint8_t inc,
					  uint8_t shift)
{
    LCD_WriteCommand(lcd, (uint8_t)(LCD_ENTRY_MODE_SET | inc | shift));
}

/**
 * @brief Configure the display control register.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] display Display on/off flag.
 * @param[in] cursor Cursor on/off flag.
 * @param[in] blink Blink on/off flag.
 *
 * @return None.
 */
void LCD_SetDisplayControl(LCD_Handle_t *lcd,
						   uint8_t display,
						   uint8_t cursor,
						   uint8_t blink)
{
    LCD_WriteCommand(lcd, (uint8_t)(LCD_DISPLAY_CTRL | display | cursor | blink));
}

/**
 * @brief Shift the display or the cursor.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] display_shift Non-zero shifts the display, zero shifts the cursor.
 * @param[in] right Non-zero shifts right, zero shifts left.
 *
 * @return None.
 */
void LCD_Shift(LCD_Handle_t *lcd,
			   uint8_t display_shift,
			   uint8_t right)
{
    uint8_t cmd = LCD_SHIFT;

    if (display_shift)
    {
        cmd |= (1U << 3);
    }

    if (right)
    {
        cmd |= (1U << 2);
    }

    LCD_WriteCommand(lcd, cmd);
}

/**
 * @brief Configure the HD44780 function set.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] lines Line configuration flag.
 * @param[in] font Font configuration flag.
 *
 * @return None.
 */
void LCD_FunctionSet(LCD_Handle_t *lcd,
					 uint8_t lines,
					 uint8_t font)
{
    LCD_WriteCommand(lcd, (uint8_t)(LCD_FUNCTION_SET | LCD_4BIT_MODE | lines | font));
}

/**
 * @brief Turn the LCD backlight on.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 */
void LCD_BacklightOn(LCD_Handle_t *lcd)
{
    lcd->port_state |= LCD_BL;
    LCD_WritePort(lcd);
}

/**
 * @brief Turn the LCD backlight off.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 */
void LCD_BacklightOff(LCD_Handle_t *lcd)
{
    lcd->port_state &= (uint8_t)~LCD_BL;
    LCD_WritePort(lcd);
}
