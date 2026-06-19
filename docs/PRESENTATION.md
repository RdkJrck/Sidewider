# Microsoft SideWinder FFB Wheel to USB: Architecture & Learning Review

## 1. Project Overview & Problem Statement
The goal of this project was to take a piece of legacy 1998 gaming hardware (The Microsoft SideWinder Force Feedback Wheel), bypass its obsolete Gameport interface, and translate its proprietary serial protocol into a modern, driverless USB Human Interface Device (HID). 

Instead of relying on unstable legacy software emulation, we built a raw, bare-metal hardware bridge using an STM32 microcontroller. 

---

## 2. Hardware Architecture: What & Why

### The Microcontroller: STM32G474RE (NUCLEO)
We selected the ARM Cortex-M4 based STM32G474RE. 
* **Why not Arduino Uno (ATmega328p)?** The SideWinder protocol requires microsecond-perfect timing and reads 33 bits of data per packet. The AVR chips run at 16 MHz and lack native USB. The STM32 runs at 170 MHz, processes reads in a fraction of a microsecond, and has a built-in Full-Speed USB peripheral.
* **5V Tolerance:** The SideWinder logic runs at 5V. Modern microcontrollers run at 3.3V. We specifically utilized the `FT` (Five-Volt Tolerant) pins on the STM32 (`PA0`, `PA1`, `PB0`, `PC1`). This completely eliminated the need for external voltage level-shifter chips.

### The Wiring Strategy
Instead of cutting the original DB-15 cable, we mapped the pins directly to the Breadboard. 
* **Why?** This preserves the original hardware and isolates electrical shorts. It also allows us to easily inject external power (5V) from a phone charger directly into the breadboard rails without risking drawing too much current from the PC's USB port, which could cause a brownout.

---

## 3. The Communication Layer (SideWinder Protocol)

The Gameport was originally designed for analog potentiometers (measuring resistance). Microsoft hijacked this port to transmit digital serial data.

### 3.1 Master/Slave Polling
The wheel is a "slave" device. It is completely silent until the Nucleo (the "master") demands data. 
* **Trigger (Output):** The Nucleo pulls the Trigger pin LOW. 
* **Clock (Input):** The wheel responds by pulsing a Clock line. 
* **Data 0, 1, 2 (Input):** On every clock pulse, the Nucleo reads 3 bits of data simultaneously.

### 3.2 The "Magic Wakeup" Sequence
When powered on, the wheel defaults to a legacy analog fallback mode. To switch it to digital multiplex mode, we had to implement a strict state machine:
* Send a 140µs pulse.
* Wait for the wheel to respond.
* Send an 865µs pulse.
* Wait.
* Send a 440µs pulse.
* **Why was this hard?** If the timing is off by even 10 microseconds, the wheel ignores it. We couldn't use standard `HAL_Delay()` because it only handles milliseconds.

### 3.3 Hardware Cycle Counters (DWT)
To achieve perfect microsecond delays for the wakeup sequence, we enabled the ARM Cortex-M4 Data Watchpoint and Trace (DWT) unit.
* **Why?** Software loops (like `for(int i=0; i<100; i++)`) are unpredictable because the compiler might optimize them away, or hardware interrupts might pause them. The DWT is an internal hardware clock running exactly at 170 MHz. By counting exact CPU cycles, our timing became mathematically perfect and immune to compiler optimizations.

---

## 4. The USB HID Abstraction

Once we extracted the raw 33-bit packet from the wheel, we had to make the modern PC understand it.

### The USB HID Standard
Instead of writing a custom Windows/Linux driver, we used the Universal Serial Bus Human Interface Device (USB HID) specification. 
We wrote a "Report Descriptor"—a block of bytecode that tells the operating system:
* *"I am a Joystick."*
* *"My first 16 bits are a Steering Axis ranging from -32767 to 32767."*
* *"My next 16 bits are an Accelerator ranging from 0 to 65535."*
* *"My last 8 bits are individual Buttons."*

### Data Transformation (Why we inverted the pedals)
The raw data from the SideWinder pedal pots ranges from `63` (foot off) to `0` (foot down). 
If we sent this raw data to the PC, the car in the game would constantly accelerate until you pressed the pedal. 
* **The Fix:** We applied math in the firmware (`63 - raw_value`) before packing the USB report. This normalizes the data to standard USB simulation controls, making it universally plug-and-play across all operating systems.

---

## 5. Debugging Methodology & Lessons Learned

The hardest part of embedded systems is that you cannot "see" electricity. When the system fails, you don't know if it's the wire, the code, the PC, or the wheel.

1. **The CLI (Command Line Interface):** We built a custom UART shell over the ST-Link so we could type commands (`PA5 1`, `PC1 R`) directly to the chip while it was running. This allowed us to query hardware state without recompiling code.
2. **The Hardware Loopback Test:** When the wheel wouldn't respond, we disconnected it and plugged the Nucleo's output pin (`D13`) directly into its input pin (`A0`). The code fired a pulse and read it. **Why?** Because it proved the microcontroller and code were flawless. This immediately isolated the bug.
3. **The Silkscreen Trap:** We learned that the white text painted on an Arduino board (`A1`) does not always map to the same internal chip pin (`PC1`) across different hardware revisions. The NUCLEO-G474RE routes `A1` to `PA1`. Trusting documentation over generic diagrams was the breakthrough that solved the project.

---

## 6. Next Steps: Force Feedback (Phase 2)
The current implementation is one-way (Wheel -> Nucleo -> PC).
To enable Force Feedback, the architecture must become bidirectional:
1. We must expand the USB HID Descriptor to include **PID (Physical Interface Device)** extensions.
2. The Nucleo must parse complex USB Output Reports containing effect data (e.g., Sine wave, 40Hz, 80% magnitude).
3. The Nucleo must translate these effects into proprietary MIDI SysEx packets.
4. The Nucleo must transmit these packets via a hardware UART over Gameport Pin 12 (MIDI TX) back into the wheel's internal motors.
