# Environmental Monitoring System (STM32)

## 1. Overview

This project implements an embedded system for real-time environmental monitoring based on an STM32 microcontroller. The system is designed to periodically collect, process, and display key environmental parameters, namely: air temperature, relative humidity, and atmospheric pressure.

The system operates autonomously and provides multiple output channels:
- visualization on a character LCD display,
- status indication using on-board LEDs,
- data transmission to an external system via UART in CSV format for logging and further analysis.

The project demonstrates practical usage of STM32 peripherals, interrupt-driven programming, and structured firmware design using a finite state machine.

---

## 2. System Architecture

### 2.1 General Concept

The system is organized as a modular embedded application with a clear separation of functional responsibilities. Its operation follows a cyclic measurement model, where each cycle includes data acquisition, processing, and output.

The architecture is based on two key principles:
- **finite state machine (FSM)** for organizing the control flow,
- **event-driven execution**, where system activity is triggered by periodic timer events.

A hardware timer generates interrupts at a fixed interval (1 Hz), setting a trigger flag. The main loop polls this flag and initiates a new measurement cycle. This approach avoids blocking delays and ensures predictable system behavior.

---

### 2.2 Functional Modules

The system consists of the following logical modules:

- **Control Module**  
  Manages the overall workflow of the system. It handles state transitions and coordinates interactions between all other modules.

- **Data Acquisition Module**  
  Communicates with the environmental sensor via I2C and retrieves measurement data. It also performs basic conversion of raw sensor values into physical quantities.

- **Data Processing Module**  
  Validates the measured data against predefined ranges and classifies the environmental state into one of three categories:
  - `NORMAL`,
  - `WARNING`,
  - `CRITICAL`.

- **Output Module**  
  Responsible for presenting the results:
  - formatted output to the LCD display,
  - CSV transmission via UART,
  - LED indication of the current system state.

- **Event Generation Module**  
  Uses a hardware timer to generate periodic interrupts that initiate measurement cycles, forming the basis of the event-driven model.

- **Error Handling Module**  
  Detects and processes runtime errors such as invalid sensor data or communication failures. It implements retry logic and transitions the system into a safe state in case of persistent faults.

---

## 3. Hardware Description

### 3.1 Main Components

The hardware platform of the system is built around the following components:

- **STM32F407G-DISC1 board (STM32F407VGT6)**  
  Serves as the core of the system. It provides processing capabilities and access to all required peripherals, including I2C, UART, GPIO, and timers.

- **BME280 Environmental Sensor**  
  A digital sensor used to measure temperature, relative humidity, and atmospheric pressure. Communication with the microcontroller is performed via the I2C interface.

- **LCD QC1602A (HD44780-compatible with PCF8574T I/O expander)**  
  A character display used for local visualization of measurement results. The PCF8574T I/O expander allows interfacing the LCD over I2C, reducing the number of required GPIO pins.

- **USB-UART Converter (PL2303TA)**  
  Provides a communication interface between the microcontroller and an external system for data transmission and logging.
  
---

### 3.2 Peripheral Mapping

The interaction between the microcontroller and external devices is implemented using the following peripherals:

- **I2C**
  - `I2C3` — used for communication with the BME280 sensor,  
  - `I2C1` — used for communication with the LCD (via PCF8574T I/O expander).

- **UART**
  - `USART2` — used to transfer data to an external system in a structured format.

- **GPIO**
  - Used for controlling on-board LEDs that indicate the current system state.

- **Timer**
  - `TIM6` — generates periodic interrupts (1 Hz) used to trigger measurement cycles.

---

## 4. Software Architecture

### 4.1 Programming Approach

The software is developed in **C** using the **STM32CubeIDE** environment. The implementation follows a low-level approach close to bare-metal programming, providing direct control over microcontroller peripherals.

Key characteristics of the programming approach:

- Minimal use of high-level libraries (HAL is used only where appropriate),
- Direct interaction with hardware through register-level configuration where possible,
- Use of interrupts for asynchronous event handling,
- Structured and modular code organization.

This approach improves understanding of the hardware and ensures predictable system behavior.

---

### 4.2 Project Structure

The project is organized into several logical directories, each responsible for a specific layer of functionality:

- **`base`**  
  Contains core system files, including the main entry point, startup code, interrupt handlers, and basic MCU configuration.

- **`devices`**  
  Includes drivers for external hardware components such as the BME280 sensor and the LCD display.

- **`interfaces`**  
  Provides low-level interfaces for communication peripherals (I2C, UART).

- **`platform`**  
  Contains platform-specific utilities such as delay functions, LED control, and timer configuration.

- **`system`**  
  Implements high-level application logic, including the system context and the finite state machine (FSM) that coordinates all modules.

---

## 5. Drivers

### 5.1 LCD Driver (HD44780 over I2C)

The LCD driver provides a complete software interface for controlling a 16×2 character display based on the HD44780 controller via an I2C port expander (PCF8574T).  
It abstracts low-level communication details and exposes a convenient high-level API for text output and display control.

The driver operates in **4-bit mode**, where each byte is transmitted as two nibbles.  
All signals (RS, RW, EN, D4–D7, backlight) are mapped to a single byte and sent via I2C.

Key design features:
- I2C-based communication using PCF8574T,
- Cached port state (minimizes unnecessary bit operations),
- Precise timing using DWT delays,
- Clear separation between low-level and high-level logic.

---

#### Internal Structure

The driver is built around:
- **Low-level layer**: I2C transmission and signal generation,  
- **Mid-level layer**: nibble/byte transfer logic,
- **High-level API**: user-facing display functions.  

All operations ultimately rely on two core primitives:
- `LCD_WriteCommand()`,
- `LCD_WriteData()`.

---

#### Public API

##### Initialization

- `LCD_InitHandle(LCD_Handle_t *lcd, uint8_t address)`  
  Initializes the driver descriptor and stores the I2C address.

- `LCD_Init(LCD_Handle_t *lcd)`  
  Performs full LCD initialization sequence (4-bit mode, display setup).

---

##### Basic Communication

- `LCD_WriteCommand(LCD_Handle_t *lcd, uint8_t command)`  
  Sends a command to the controller.

- `LCD_WriteData(LCD_Handle_t *lcd, uint8_t data)`  
  Sends one byte of data (character).

---

##### Display Control

- `LCD_Clear(LCD_Handle_t *lcd)`  
  Clears the display.

- `LCD_Home(LCD_Handle_t *lcd)`  
  Returns cursor to home position.

- `LCD_SetDisplayControl(LCD_Handle_t *lcd, uint8_t display, uint8_t cursor, uint8_t blink)`  
  Controls display, cursor visibility, and blinking.

- `LCD_SetEntryMode(LCD_Handle_t *lcd, uint8_t inc, uint8_t shift)`  
  Configures cursor movement behavior.

- `LCD_Shift(LCD_Handle_t *lcd, uint8_t display_shift, uint8_t right)`  
  Shifts display or cursor.

- `LCD_FunctionSet(LCD_Handle_t *lcd, uint8_t lines, uint8_t font)`  
  Configures display mode (lines, font).

---

##### Cursor and Memory Control

- `LCD_SetCursor(LCD_Handle_t *lcd, uint8_t row, uint8_t col)`  
  Sets cursor position using row/column.

- `LCD_SetDDRAM(LCD_Handle_t *lcd, uint8_t addr)`  
  Directly sets DDRAM address.

---

##### Text Output

- `LCD_PutChar(LCD_Handle_t *lcd, char c)`  
  Outputs a single character.

- `LCD_Print(LCD_Handle_t *lcd, const char *str)`  
  Outputs a null-terminated string.

- `LCD_Printf(LCD_Handle_t *lcd, const char *fmt, ...)`  
  Outputs formatted text (printf-style).

---

##### Backlight Control

- `LCD_BacklightOn(LCD_Handle_t *lcd)`  
  Enables backlight.

- `LCD_BacklightOff(LCD_Handle_t *lcd)`  
  Disables backlight.

---

#### Summary

The LCD driver encapsulates:
- 4-bit data transmission,
- I2C communication via expander,
- Timing constraints handling,
- High-level text rendering.

This makes it reusable and independent from the rest of the system logic.

---

### 5.2 BME280 Sensor Driver

The BME280 driver provides a complete software interface for interacting with the digital environmental sensor over the I2C interface.  
It encapsulates all low-level communication details and exposes a structured high-level API for initialization, configuration, and data acquisition.

The driver is designed to be modular, reusable, and hardware-agnostic (within the STM32 HAL ecosystem), making it suitable for integration into embedded systems requiring reliable environmental sensing.

Key design features:
- I2C-based communication using STM32 HAL,
- Support for all operating modes (sleep, forced, normal),
- Automatic loading of calibration coefficients,
- Fixed-point compensation algorithms (Bosch compliant),
- Configurable oversampling, filtering, and standby time,
- Separation of raw and compensated data handling,
- Clean abstraction layers for maintainability.

---

#### Internal Structure

The driver is organized into three logical layers:

##### Low-Level Layer

Handles direct register access via I2C:

- `BME280_I2C_Read()`  
  Reads one or more bytes from a sensor register.

- `BME280_I2C_Write()`  
  Writes a single byte to a sensor register.

Wrapper functions:

- `BME280_ReadRegister()`,  
- `BME280_WriteRegister()`.  

These functions operate on the device handle and abstract I2C details.

---

##### Data Processing Layer

Responsible for:

- Reading calibration data:
  - `BME280_ReadCalibration()`.
- Measurement control:
  - `BME280_WaitMeasurement()`,
  - `BME280_ForceMeasurement()`.
- Compensation algorithms:
  - `BME280_CompensateTemperature()`,
  - `BME280_CompensatePressure()`,
  - `BME280_CompensateHumidity()`.

These functions convert raw ADC values into accurate physical quantities using sensor-specific calibration coefficients.

---

##### High-Level API

Provides user-facing functionality:

###### Initialization

- `BME280_Init(BME280_Handle_t *handle)`  
  Performs full initialization:
  - chip ID verification,
  - sensor reset,
  - calibration loading,  
  - default configuration,  
  - mode setup. 

- `BME280_Reset(BME280_Handle_t *handle)`  
  Performs a software reset.

---

###### Operating Modes

- `BME280_SetMode(BME280_Handle_t *handle, uint8_t mode)`
- `BME280_StartNormal(BME280_Handle_t *handle)`  
- `BME280_Sleep(BME280_Handle_t *handle)`

---

###### Configuration

- `BME280_Configure(BME280_Handle_t *handle, const BME280_Config_t *cfg)`  
- `BME280_SetOversampling(...)`  
- `BME280_SetFilter(...)`  
- `BME280_SetStandby(...)`

---

###### Data Acquisition

- `BME280_ReadRaw(...)`  
  Reads raw ADC values.

- `BME280_ReadAll(...)`  
  Returns compensated data.

- `BME280_ReadForced(...)`  
  Performs a single forced measurement.

- `BME280_ReadNormal(...)`  
  Reads data in normal mode.

---

###### Unit Conversion

- `BME280_ConvertTemperature(...)`  
- `BME280_ConvertPressure(...)`  
- `BME280_ConvertHumidity(...)`

---

#### Data Structures

- `BME280_Handle_t`  
  Device descriptor:
  - I2C interface,  
  - device address,  
  - calibration data,  
  - internal `t_fine` variable.  

- `BME280_CalibData_t`  
  Stores calibration coefficients read from the sensor.

- `BME280_Data_t`  
  Contains processed environmental values:
  - temperature (°C),  
  - pressure (hPa),  
  - humidity (%).

- `BME280_Config_t`  
  Configuration parameters:
  - oversampling,  
  - filter,  
  - standby time.  

---

#### Summary

The BME280 driver:
- abstracts all hardware communication details,
- implements complete sensor functionality,
- provides a clear and structured API,
- ensures accurate and reliable measurements.

This makes it a robust and reusable component within the environmental monitoring system.

---

## 6. Build and Run
The project is intended to be built and executed using **STM32CubeIDE**.

### 6.1 Build Steps
1. Open **STM32CubeIDE**.
2. Import the project:
- File → Import → Existing Projects into Workspace.
3. Select the project root directory.
4. Click **Build Project**.

---

### 6.2 Flash and Run
1. Connect the STM32F407G-DISC1 board via USB.
2. Click **Run** or **Debug** in STM32CubeIDE.
3. The firmware will be flashed to the microcontroller.
4. After startup, the system begins autonomous operation.

---

### 6.3 Output Monitoring
- **LCD Display** — shows real-time environmental data
- **LEDs** — indicate system state (`NORMAL`, `WARNING`, `CRITICAL`)
- **UART (USART2)** — transferring data to an external system (such as a PC)

To view UART output:
- connect a USB-UART converter,
- open a serial terminal (e.g., PuTTY, Tera Term),
- configure the correct COM port and baud rate.

---

## 7. Summary
The project represents a complete embedded system for environmental monitoring, combining:
- hardware interfacing,
- real-time data acquisition,
- structured software design,
- event-driven execution,
- and reliable error handling.

It serves both as a functional prototype and as an educational example of designing microcontroller-based systems using STM32.