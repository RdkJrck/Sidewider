# Sidewider — Microsoft SideWinder FFB Wheel → USB Converter

A bare-metal STM32G474RE firmware that reads data from a **Microsoft SideWinder Force Feedback Wheel** (gameport era) via its proprietary serial protocol and re-enumerates it as a standard **USB HID gamepad/racing wheel** on a modern PC — no drivers required.

> **Status:** Phase 1–3 working (USB enumeration + SideWinder read + HID report). Phase 4 (Force Feedback / HID PID) is the next milestone.

---

## Table of Contents

1. [Project Goals](#project-goals)
2. [Hardware](#hardware)
3. [Wiring](#wiring)
4. [Software Architecture](#software-architecture)
5. [SideWinder Protocol](#sidewinder-protocol)
6. [USB HID Report Format](#usb-hid-report-format)
7. [Clock & USB Configuration](#clock--usb-configuration)
8. [Build Instructions](#build-instructions)
9. [Flashing](#flashing)
10. [LED Status Codes](#led-status-codes)
11. [Project Structure](#project-structure)
12. [Known Issues & TODO](#known-issues--todo)
13. [References](#references)

---

## Project Goals

| # | Goal | Status |
|---|------|--------|
| 1 | STM32G474RE recognised as USB HID gamepad | ✅ Done |
| 2 | Implement Microsoft SideWinder gameport read protocol | ✅ Done |
| 3 | Bridge SideWinder data to USB HID reports | ✅ Done |
| 4 | Implement HID PID Force Feedback output | 🔲 Planned |

---

## Hardware

| Component | Detail |
|-----------|--------|
| MCU board | **NUCLEO-G474RE** (STM32G474RE, ARM Cortex-M4 @ 170 MHz) |
| USB peripheral | Internal USB Full-Speed controller (PA11/PA12) |
| Legacy wheel | **Microsoft SideWinder Force Feedback Wheel** (gameport DB-15) |

---

## Wiring

The SideWinder communicates over a 3-wire synchronous serial bus exposed on the gameport connector. Connect it to the Nucleo's **CN10 Arduino header**:

| SideWinder Signal | Gameport Pin | Nucleo Header | STM32 GPIO |
|-------------------|-------------|---------------|------------|
| CLOCK             | Pin 2       | A0 (CN8)      | PA0        |
| TRIGGER (Output)  | Pin 3       | D13 (CN9)     | PA5 (LED)  |
| DATA 0            | Pin 7       | A1 (CN8)      | PA1        |
| DATA 1            | Pin 10      | A3 (CN8)      | PB0        |
| DATA 2            | Pin 14      | A4 (CN8)      | PC1        |
| GND               | Pin 4/12    | GND           | GND        |
| +5V               | Pin 1       | +5V           | —          |

> **Note:** The STM32G474RE has a different internal routing for `A0-A4` compared to a standard Arduino Uno. Do not trust generic pinout diagrams; use the exact GPIOs listed above. PA11 / PA12 are reserved for USB (DM / DP).

---

## Software Architecture

```
main.c
 ├─ SystemClock_Config()   — PLL @ 170 MHz, HSI48 for USB, CRS trim
 ├─ MX_GPIO_Init()         — PA5 (LED), GPIOA clock
 ├─ MX_USB_Device_Init()   — USB stack init + DP pull-up
 ├─ SideWinder_Init()      — GPIO setup for SW bus (PC0/PC1/PC2)
 └─ while(1) @ 10 ms
       ├─ SideWinder_Read()    — pull 33+ bits from wheel
       └─ USBD_HID_SendReport() — push Gamepad_Report_t over USB

USB Stack (STM32 USB Device Library)
 ├─ usb_device.c       — USBD_Init / RegisterClass / Start
 ├─ usbd_conf.c        — HAL PCD glue, PMA allocation, IRQ handler
 ├─ usbd_desc.c        — Device / Config / String descriptors (VID:PID 0483:5710)
 └─ usbd_hid_gamepad.c — Custom HID class (Init/Setup/DataIn/SendReport)

SideWinder Driver
 └─ sidewinder.c / sidewinder.h
```

---

## SideWinder Protocol

The Microsoft SideWinder FFB Wheel uses a **synchronous serial** protocol over the gameport interface, triggered by a host-generated strobe pulse.

### Timing Sequence

1. **Strobe HIGH** for ~25 µs — triggers the wheel to start clocking out data.
2. **Strobe LOW** — wheel begins sending bits.
3. Host waits for first **CLOCK HIGH**, then enters bit-capture loop:
   - Wait for **falling edge** of CLOCK.
   - Sample **DATA** line.
   - Wait for **rising edge** of CLOCK (or timeout → end of packet).
4. Repeat until **≥ 33 bits** received or a timeout occurs.

### Bit Layout (FFB Wheel — 33 bits minimum)

| Bits | Field    | Width | Range  | Notes                    |
|------|----------|-------|--------|--------------------------|
| 0–9  | Steering | 10 b  | 0–1023 | LSB first (512 is center)|
| 10–15| Brake    | 6 b   | 0–63   | Right Pedal (63=idle, 0=pressed) |
| 16–21| Gas      | 6 b   | 0–63   | Left Pedal (63=idle, 0=pressed)  |
| 22–29| Buttons  | 8 b   | bitmask| Buttons 1–8              |
| 30–32| Padding/Parity | 3 b | — | Parity validation        |

### Button Bitmask (Bits 22-29)
| Button | Bit | Hex |
|---|---|---|
| A | 0 | `0x01` |
| B | 1 | `0x02` |
| C | 2 | `0x04` |
| Right Paddle | 3 | `0x08` |
| X | 4 | `0x10` |
| Y / V | 5 | `0x20` |
| Z | 6 | `0x40` |
| Left Paddle | 7 | `0x80` |

> **Implementation note:** Interrupt masking was removed from `SideWinder_Read()` because it would block the USB interrupt handler and cause USB enumeration failures. The software timeout loops act as the guard against a missing/desynchronised wheel.

---

## USB HID Report Format

The device enumerates as **VID 0x0483 / PID 0x5710** ("Sidewinder-USB / FFB Wheel Converter").

### `Gamepad_Report_t` (7 bytes, packed)

| Offset | Field       | Type     | HID Usage              | Range           |
|--------|-------------|----------|------------------------|-----------------|
| 0x00   | steering    | int16_t  | Generic Desktop – X    | −32767…32767    |
| 0x02   | accelerator | uint16_t | Simulation – Accelerator | 0…65535       |
| 0x04   | brake       | uint16_t | Simulation – Brake     | 0…65535         |
| 0x06   | buttons     | uint8_t  | Button 1–8 (bitmask)   | 0x00…0xFF       |

### Axis scaling

```c
/* Steering: 10-bit (0–1023) → int16 signed */
myGamepad.steering = (int16_t)(((int32_t)swData.steering - 512) * 64);

/* Pedals: 6-bit (0–63) → uint16 */
myGamepad.accelerator = (uint16_t)(swData.gas   * 1040);
myGamepad.brake       = (uint16_t)(swData.brake * 1040);
```

HID reports are sent every **10 ms** (100 Hz) if `swData.valid == 1`.

---

## Clock & USB Configuration

| Parameter | Value |
|-----------|-------|
| PLL source | HSI (16 MHz) |
| PLLM | /4 → 4 MHz |
| PLLN | ×85 → 340 MHz VCO |
| PLLR | /2 → **170 MHz SYSCLK** |
| USB clock | HSI48, trimmed by **CRS** (Clock Recovery System) synchronised to USB SOF |
| Flash latency | 4 wait states |

The CRS ensures the USB clock stays within ±500 ppm of the USB 12 MHz reference, even on a free-running HSI48.

---

## Build Instructions

### Prerequisites

```bash
# Arch / Manjaro
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib

# Debian / Ubuntu
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi
```

### Build

```bash
git clone https://github.com/RdkJrck/Sidewider.git
cd Sidewider
make
```

Output files are placed in `build/`:
- `build/main.elf` — debug ELF with DWARF symbols
- `build/main.map` — linker map (symbol sizes, section layout)

```bash
make clean   # remove build artefacts
make debug   # print resolved Makefile variables and src listing
```

---

## Flashing

The default flash handler is **STM32CubeProgrammer**. Ensure `STM32_Programmer_CLI` is in your system `$PATH`.

You can simply flash and reset the device automatically by running:

```bash
make flash
```

Alternatively, to run the process manually:

```bash
make clean
make
STM32_Programmer_CLI -c port=SWD -w build/main.elf 0x08000000 -v -rst
```

---

## LED Status Codes

The built-in LED on **PA5** (LD2 on NUCLEO-G474RE) is used for boot diagnostics:

| Pattern | Meaning |
|---------|---------|
| 3 slow blinks | Firmware started (before clock config) |
| 1 short blink | System clock configured (170 MHz) |
| 1 short blink | USB stack initialised |
| Fast rapid toggle (infinite) | `Error_Handler()` — init failure |
| Toggle on every USB IRQ | USB interrupt activity (diagnostic, can be removed) |

---

## Project Structure

```
Sidewider/
├── src/
│   ├── main.c                 # Entry point: clock/GPIO/USB/SideWinder init + main loop
│   ├── sidewinder.c           # SideWinder gameport read driver
│   ├── usb_device.c           # USB library init (USBD_Init / RegisterClass / Start)
│   ├── usbd_conf.c            # HAL PCD glue layer + PMA config + IRQ handler
│   ├── usbd_desc.c            # USB device/config/string descriptors
│   ├── usbd_hid_gamepad.c     # Custom HID class implementation + SendReport
│   └── stm32g4xx_it.c         # Interrupt vector table (mostly empty stubs)
├── include/
│   ├── main.h                 # HAL includes, Error_Handler prototype
│   ├── sidewinder.h           # SideWinder GPIO pin defs + SideWinder_Data_t struct
│   ├── usbd_gamepad_report.h  # HID report descriptor + Gamepad_Report_t struct
│   ├── usbd_hid_gamepad.h     # USBD_HID_Gamepad class handle + SendReport prototype
│   ├── usbd_conf.h            # USB library configuration macros
│   ├── usbd_desc.h            # Descriptor function prototypes
│   ├── usb_device.h           # MX_USB_Device_Init prototype
│   ├── stm32g474xx_flash.ld   # Linker script (FLASH 512K @ 0x08000000, RAM 128K)
│   └── stm32g4xx_hal_conf.h   # HAL module enable switches
├── STM32CubeG4/               # ST Microelectronics SDK (HAL + USB Middleware)
├── cmsis_inc/                 # Extra CMSIS headers (wint_t workaround)
├── newlib_include/            # Newlib headers (nano.specs)
├── Makefile                   # arm-none-eabi-gcc build system
├── .clangd                    # clangd LSP config for IDE support
└── README.md                  # This file
```

---

## Testing on Linux

Since the Nucleo enumerates as a standard USB HID Joystick, no custom drivers are required. You can test the inputs natively using standard Linux utilities.

1. **Verify USB Enumeration:**
   ```bash
   lsusb | grep 0483
   # Expected output: ID 0483:5710 STMicroelectronics Joystick in FS Mode
   ```

2. **Test Raw Inputs (Arch Linux example):**
   Install the joystick utilities:
   ```bash
   sudo pacman -S evtest joyutils
   ```
   Run `evtest` and select the device corresponding to the `STMicroelectronics Joystick`:
   ```bash
   evtest
   ```
   You will see a live stream of raw kernel input events as you turn the wheel, press the pedals, and click the buttons.

---

## Known Issues & TODO

### Known Issues
- `USBD_HID_REPORT_DESC_SIZE` in `usbd_hid_gamepad.h` is hardcoded as `74` — should use `sizeof(HID_GAMEPAD_ReportDesc)` to avoid silent descriptor mismatch.

### Roadmap
- [ ] **Phase 4:** Implement HID PID (Physical Interface Device) for Force Feedback via MIDI (Gameport Pin 15 / `MIDI_TX`). Auto-centering currently works natively out of the box.
- [ ] Add parity/checksum validation on the 33-bit SideWinder packet
- [x] Replace spin-loop `delay_us()` with DWT cycle counter for microsecond protocol accuracy
- [x] Add a `make flash` target to the Makefile
- [x] Add UART debug output via PA2 (LPUART1 on NUCLEO) with interactive diagnostic commands

---

## References

- [necroware/gameport-adapter](https://github.com/necroware/gameport-adapter) — SideWinder protocol reverse-engineering reference
- [STM32G4 Series Reference Manual (RM0440)](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32CubeG4 SDK](https://github.com/STMicroelectronics/STM32CubeG4)
- [USB HID Usage Tables 1.3](https://usb.org/document-library/hid-usage-tables-13)
- [HID PID Specification 1.0](https://usb.org/document-library/device-class-definition-physical-interface-devices-pid-10)
