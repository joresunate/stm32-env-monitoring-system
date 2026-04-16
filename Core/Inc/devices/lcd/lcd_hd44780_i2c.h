/**
 * @file lcd_hd44780_i2c.h
 * @brief Public interface for the HD44780 character LCD driver over I2C.
 *
 * This header defines the LCD handle, configuration constants, and the
 * high-level API used to initialize the display, position the cursor,
 * control the backlight, and print text through a PCF8574 I/O expander.
 */

#ifndef LCD_HD44780_I2C
#define LCD_HD44780_I2C

#include <stdint.h>
#include <stdarg.h>

/**
 * @brief Default 7-bit I2C address of the PCF8574 expander.
 *
 * This value matches the most common address configuration
 * used by LCD adapter boards based on PCF8574T.
 */
#define LCD_DEFAULT_I2C_ADDR  0x27U

/**
 * @brief Minimum delay after an HD44780 command transfer, in microseconds.
 *
 * This delay satisfies the execution time requirements for standard
 * LCD commands in the 4-bit interface mode.
 */
#define LCD_CMD_DELAY_US  50U

/**
 * @brief Internal buffer size used by the printf-style printing routine.
 *
 * Formatted output is truncated if the resulting string exceeds this buffer.
 */
#define LCD_PRINTF_BUFFER_SIZE  128U

/**
 * @brief Visible LCD geometry supported by the driver.
 *
 */
#define LCD_COLS  16U
#define LCD_ROWS  2U

/**
 * @brief PCF8574 pin mapping to the LCD control and data signals.
 *
 * The mapping corresponds to the typical adapter board wiring used with
 * HD44780-compatible character LCD modules.
 */
#define LCD_RS   (1U << 0)  /**< Register Select line. */
#define LCD_RW   (1U << 1)  /**< Read/Write line; kept low in this driver. */
#define LCD_EN   (1U << 2)  /**< Enable strobe line. */
#define LCD_BL   (1U << 3)  /**< Backlight control line. */

#define LCD_D4   (1U << 4)  /**< LCD data line D4. */
#define LCD_D5   (1U << 5)  /**< LCD data line D5. */
#define LCD_D6   (1U << 6)  /**< LCD data line D6. */
#define LCD_D7   (1U << 7)  /**< LCD data line D7. */

/**
 * @brief Register selection flags used by the write routines.
 */
#define LCD_CMD   0x00U   /**< Command register mode. */
#define LCD_DATA  LCD_RS  /**< Data register mode. */

/**
 * @brief HD44780 base command values.
 *
 * These constants represent the command groups defined
 * by the HD44780 instruction set.
 */
#define LCD_CLEAR_DISPLAY    0x01U
#define LCD_RETURN_HOME      0x02U
#define LCD_ENTRY_MODE_SET   0x04U
#define LCD_DISPLAY_CTRL     0x08U
#define LCD_SHIFT            0x10U
#define LCD_FUNCTION_SET     0x20U
#define LCD_SET_DDRAM        0x80U

/**
 * @brief Entry mode configuration flags.
 *
 * These flags are combined with LCD_ENTRY_MODE_SET.
 */
#define LCD_ENTRY_INC        0x02U  /**< Increment DDRAM address after each write. */
#define LCD_ENTRY_DEC        0x00U  /**< Decrement DDRAM address after each write. */
#define LCD_ENTRY_SHIFT_ON   0x01U  /**< Shift display when writing a character. */
#define LCD_ENTRY_SHIFT_OFF  0x00U  /**< Do not shift display when writing a character. */

/**
 * @brief Display control configuration flags.
 *
 * These flags are combined with LCD_DISPLAY_CTRL.
 */
#define LCD_DISPLAY_ON   0x04U  /**< Enable display output. */
#define LCD_DISPLAY_OFF  0x00U  /**< Disable display output. */
#define LCD_CURSOR_ON    0x02U  /**< Enable visible cursor. */
#define LCD_CURSOR_OFF   0x00U  /**< Disable visible cursor. */
#define LCD_BLINK_ON     0x01U  /**< Enable cursor blinking. */
#define LCD_BLINK_OFF    0x00U  /**< Disable cursor blinking. */

/**
 * @brief Function set configuration flags.
 *
 * These flags are combined with LCD_FUNCTION_SET.
 */
#define LCD_4BIT_MODE    0x00U  /**< 4-bit interface mode. */
#define LCD_1LINE        0x00U  /**< One display line. */
#define LCD_2LINE        0x08U  /**< Two display lines. */
#define LCD_5x8_DOTS     0x00U  /**< 5x8 dot character font. */
#define LCD_5x11_DOTS    0x04U  /**< 5x11 dot character font. */

/**
 * @brief LCD driver descriptor.
 *
 * The handle stores the PCF8574 I2C address and the cached state of the
 * output port, which includes both control lines and data lines.
 */
typedef struct
{
    uint8_t address;     /**< I2C address of the PCF8574 expander. */
    uint8_t port_state;  /**< Cached state of the expander output port. */

} LCD_Handle_t;

/**
 * @brief Initialize the LCD descriptor.
 *
 * @param[out] lcd Pointer to the LCD descriptor structure to initialize.
 * @param[in] address 7-bit I2C address of the PCF8574 expander.
 *
 * @return None.
 */
void LCD_InitHandle(LCD_Handle_t *lcd, uint8_t address);

/**
 * @brief Initialize the LCD in 4-bit mode using the HD44780 power-up sequence.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 *
 * @note This routine is blocking and depends on the DWT-based delay module.
 */
void LCD_Init(LCD_Handle_t *lcd);

/**
 * @brief Send a command byte to the LCD controller.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] command Command byte to transmit.
 *
 * @return None.
 */
void LCD_WriteCommand(LCD_Handle_t *lcd, uint8_t command);

/**
 * @brief Send a data byte to the LCD controller.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] data Data byte to transmit.
 *
 * @return None.
 */
void LCD_WriteData(LCD_Handle_t *lcd, uint8_t data);

/**
 * @brief Clear the display and reset the DDRAM address to home.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 *
 * @note The command requires a longer execution time than most other
 *       instructions, therefore the implementation inserts a delay.
 */
void LCD_Clear(LCD_Handle_t *lcd);

/**
 * @brief Return the cursor to the home position.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 *
 * @note The command requires a longer execution time than most other
 *       instructions, therefore the implementation inserts a delay.
 */
void LCD_Home(LCD_Handle_t *lcd);

/**
 * @brief Set the cursor position using row and column coordinates.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] row Target row index. Valid values are 0 and 1.
 * @param[in] col Target column index.
 *
 * @return None.
 *
 * @note If the row index is out of range, the function returns without
 *       sending a command. Column values are forwarded to the DDRAM
 *       address calculation as allowed by the controller.
 */
void LCD_SetCursor(LCD_Handle_t *lcd, uint8_t row, uint8_t col);

/**
 * @brief Set the DDRAM address directly.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] address DDRAM address to select.
 *
 * @return None.
 *
 * @note Only addresses in the visible ranges handled by the driver are
 *       accepted. Invalid addresses are ignored.
 */
void LCD_SetDDRAM(LCD_Handle_t *lcd, uint8_t address);

/**
 * @brief Print a single character at the current cursor position.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] c Character to write.
 *
 * @return None.
 */
void LCD_PutChar(LCD_Handle_t *lcd, char c);

/**
 * @brief Print a null-terminated string at the current cursor position.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] str Pointer to the null-terminated string.
 *
 * @return None.
 */
void LCD_Print(LCD_Handle_t *lcd, const char *str);

/**
 * @brief Print formatted text using printf-style formatting.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] fmt Format string.
 *
 * @return None.
 *
 * @note The output is limited by LCD_PRINTF_BUFFER_SIZE and may be truncated
 *       if the formatted string does not fit into the internal buffer.
 */
void LCD_Printf(LCD_Handle_t *lcd, const char *fmt, ...);

/**
 * @brief Enable the LCD backlight.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 */
void LCD_BacklightOn(LCD_Handle_t *lcd);

/**
 * @brief Disable the LCD backlight.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 *
 * @return None.
 */
void LCD_BacklightOff(LCD_Handle_t *lcd);

/**
 * @brief Configure the entry mode of the LCD controller.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] inc Increment or decrement mode flag.
 * @param[in] shift Display shift enable or disable flag.
 *
 * @return None.
 *
 * @note The expected values are LCD_ENTRY_INC or LCD_ENTRY_DEC for @p inc,
 *       and LCD_ENTRY_SHIFT_ON or LCD_ENTRY_SHIFT_OFF for @p shift.
 */
void LCD_SetEntryMode(LCD_Handle_t *lcd, uint8_t inc, uint8_t shift);

/**
 * @brief Configure the display control flags.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] display Display enable or disable flag.
 * @param[in] cursor Cursor enable or disable flag.
 * @param[in] blink Cursor blink enable or disable flag.
 *
 * @return None.
 *
 * @note The expected values are LCD_DISPLAY_ON or LCD_DISPLAY_OFF for
 *       @p display, LCD_CURSOR_ON or LCD_CURSOR_OFF for @p cursor, and
 *       LCD_BLINK_ON or LCD_BLINK_OFF for @p blink.
 */
void LCD_SetDisplayControl(LCD_Handle_t *lcd, uint8_t display, uint8_t cursor, uint8_t blink);

/**
 * @brief Shift the display or the cursor.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] display_shift Non-zero shifts the display; zero shifts the cursor.
 * @param[in] right Non-zero shifts to the right; zero shifts to the left.
 *
 * @return None.
 */
void LCD_Shift(LCD_Handle_t *lcd, uint8_t display_shift, uint8_t right);

/**
 * @brief Configure the HD44780 function set command.
 *
 * @param[in,out] lcd Pointer to the LCD descriptor structure.
 * @param[in] lines Line configuration flag.
 * @param[in] font Font size flag.
 *
 * @return None.
 *
 * @note The driver always uses 4-bit mode internally. The @p lines parameter
 *       should be selected from LCD_1LINE or LCD_2LINE, and @p font from
 *       LCD_5x8_DOTS or LCD_5x11_DOTS.
 */
void LCD_FunctionSet(LCD_Handle_t *lcd, uint8_t lines, uint8_t font);

#endif
