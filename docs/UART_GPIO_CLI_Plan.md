# Interactive UART GPIO CLI

The goal is to create a simple, reusable Command Line Interface (CLI) over the existing UART connection. This will allow you to type commands in `screen` to instantly configure and toggle any pin on the Nucleo board.

## Proposed Implementation

We will implement a small non-blocking UART receiver in the `main.c` `while(1)` loop. 

When you type a command, you can either Write or Read a pin:

**To Write (Set High/Low):**
Type `PA5 1` or `PA5 0`. The firmware will:
1. Parse the port (`A`), the pin number (`5`), and the desired state (`1`).
2. Automatically configure that specific pin as a GPIO Output.
3. Set the pin to High (1) or Low (0).
4. Print a confirmation.

**To Read (Listen):**
Type `PA5 R`. The firmware will:
1. Parse the port (`A`) and pin number (`5`).
2. Automatically configure the pin as a GPIO Input (with pull-up or no-pull).
3. Read the current voltage level (High or Low).
4. Print the state back to your terminal (`PA5 is HIGH`).

### `main.c` Changes
#### [MODIFY] `main.c`
- Add a `Process_CLI_Command(char* cmd)` function.
- Support `sscanf` formats to detect both Write (`P%c%d %d`) and Read (`P%c%d R`).
- Add a small translation function to convert `'A'` to the `GPIOA` memory register and `5` to `GPIO_PIN_5`.
- Add non-blocking `HAL_UART_Receive` inside the `while(1)` loop to capture keystrokes without stopping the rest of the SideWinder code.

## Verification Plan
1. Compile and flash the new code.
2. Open `screen`.
3. Type `PA5 1` to turn on the onboard LED, and `PA5 0` to turn it off.
4. Type `PA5 R` to read the state of the pin.
5. You can connect any pin (like `PB3`) to Ground or 3.3V, type `PB3 R`, and see the live value.

## User Review Required
> [!NOTE]
> This polling approach is simple and perfect for debugging. For production systems with heavy traffic, we would use Interrupts or DMA, but polling is much safer here so we don't interfere with the sensitive USB/Gameport interrupts. Does this approach look good to you?
