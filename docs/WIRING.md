# SideWinder FFB Wheel → Nucleo-G474RE Wiring Reference

**Board:** NUCLEO-G474RE (MB1367-G474RE-C04)  
**Protocol source:** necroware/gameport-adapter `Sidewinder.h` + `GamePort.h`  
**Status:** Phase 1–3 (read + USB HID). Phase 4 (Force Feedback) notes included.

---

## DB-15 Gameport Connector Pinout

Pin numbering on the **female** DB-15 connector (viewed from the front, facing the pins):

```
 ┌─────────────────────────────────┐
 │  1   2   3   4   5   6   7   8  │  ← top row
 │    9  10  11  12  13  14  15    │  ← bottom row
 └─────────────────────────────────┘
```

| Pin | Standard Gameport Function | SideWinder Digital Role | Soldered on wheel board? |
|:---:|:---|:---|:---:|
| 1  | +5V                        | Logic power for wheel MCU        | ✅ |
| 2  | Button 1 (digital input)   | **CLOCK** — clocked by wheel     | ✅ |
| 3  | Joystick 1 X-axis (analog) | **TRIGGER** — output from Nucleo | ✅ |
| 4  | GND                        | Signal ground                    | ✅ |
| 5  | GND (duplicate)            | —                                | ❌ absent on wheel board |
| 6  | Joystick 1 Y-axis (analog) | Not used (digital mode)          | ❌ absent on wheel board |
| 7  | Button 2 (digital input)   | **DATA 0** — primary data        | ✅ |
| 8  | +5V (duplicate)            | —                                | ❌ absent on wheel board |
| 9  | +5V (duplicate)            | —                                | ❌ absent on wheel board |
| 10 | Button 3 (digital input)   | **DATA 1** — 3-bit mode ch2      | ✅ |
| 11 | Joystick 2 X-axis (analog) | Not used                         | ✅ (ignore, leave unwired) |
| 12 | MIDI OUT (PC → device)     | **Force Feedback TX** (Phase 4)  | ✅ |
| 13 | Joystick 2 Y-axis (analog) | Not used                         | ❌ absent |
| 14 | Button 4 (digital input)   | **DATA 2** — 3-bit mode ch3      | ✅ |
| 15 | MIDI IN (device → PC)      | Not used (FF is one-way)         | ❌ absent on wheel board |
| Shell | Chassis GND             | Shield / common GND              | ✅ |

---

## Wiring Table: SideWinder → Nucleo-G474RE

> **5V tolerance:** PA0, PA1, PB0, PC1 are **FT (5V-tolerant)** pins on the STM32G474RE.  
> The wheel logic runs at 5V; the Nucleo GPIO inputs safely accept up to 5.5V — **no level shifter needed**.  
> The Nucleo TRIGGER output is 3.3V which is above TTL V_IH_min (2.0V) — **accepted by the wheel without modification**.

### Signal wires (5 wires)

| SideWinder Signal | Gameport Pin | Nucleo Connector | Nucleo Label | STM32 Pin | Direction |
|:---|:---:|:---:|:---:|:---:|:---:|
| **CLOCK**         | Pin 2        | CN8, Pin 1       | **A0**       | **PA0**   | IN        |
| **TRIGGER**       | Pin 3        | CN9, Pin 5       | **D13**      | **PA5**   | OUT       |
| **DATA 0**        | Pin 7        | CN8, Pin 2       | **A1**       | **PA1**   | IN        |
| **DATA 1**        | Pin 10       | CN8, Pin 4       | **A3**       | **PB0**   | IN        |
| **DATA 2**        | Pin 14       | CN8, Pin 5       | **A4**       | **PC1**   | IN        |

**Locating CN8 (Signals) and CN9 (Trigger):**
CN8 is the small 6-pin header on the left side. CN9 is the top right header. Look for the D13 silkscreen label.

### Power wires (3 wires)

| SideWinder Signal | Gameport Pin | Nucleo Connector | Nucleo Label |
|:---|:---:|:---|:---:|
| +5V logic power | Pin 1   | CN6 pin 5 | **+5V** |
| GND             | Pin 4   | CN6 pin 6 | **GND** |
| Chassis GND     | Shell   | CN6 pin 7 | **GND** |

**Locating CN6 (Power):**
CN6 is the 8-pin power header, just above CN8 on the left side of the board.
*(Top of the header = pin 1, marked with a square pad on the bottom)*
- Pin 1 ── NC
- Pin 2 ── IOREF
- Pin 3 ── NRST
- Pin 4 ── +3.3V
- Pin 5 ── **+5V**  ← Wire to SideWinder Pin 1
- Pin 6 ── **GND**  ← Wire to SideWinder Pin 4 + Shell
- Pin 7 ── **GND**  ← (Optional duplicate Ground)
- Pin 8 ── VIN
### Leave disconnected

| Gameport Pin | Reason |
|:---:|:---|
| Pin 5, 8, 9 | Duplicate +5V/GND — redundant |
| Pin 6, 13   | Analog axis — irrelevant in digital mode |
| Pin 11      | Analog axis — irrelevant (soldered on board but leave unwired at breadboard) |
| Pin 15      | MIDI IN — FF is PC→wheel only, not needed |

---

## REAL INFO 
Breadboard Row 1 = +5V
Breadboard Row 2 = CLK (Nucleo A0 / PA0)
Breadboard Row 3 = Trigger (Nucleo D13 / PA5)
Breadboard Row 4 = GND
Breadboard Row 7 = DATA 0 (Nucleo A1 / PA1)
Breadboard Row 10 = DATA 1 (Nucleo A3 / PB0)
Breadboard Row 14 = DATA 2 (Nucleo A4 / PC1)

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

## Do You Need Resistors or Capacitors?

**Short answer: one capacitor only, zero resistors.**

| Component | Needed? | Reason |
|:---|:---:|:---|
| Pull-up resistors on DATA/CLK lines | ❌ No | STM32 internal ~40 kΩ pull-ups are already enabled in firmware on PC0/PC1/PC3/PC4 |
| Series resistors (33–100 Ω) on signal lines | ❌ No | Only needed for cables >30 cm to suppress signal ringing. Adds unwanted RC delay at 8 cm |
| Voltage divider / level shifter | ❌ No | PC0/PC1/PC3/PC4 are 5V-tolerant FT pins; TRIGGER at 3.3V is within TTL spec |
| **100 nF ceramic capacitor on +5V/GND rail** | ⚠️ Recommended | The wheel's DC motor (even at rest) injects switching noise into the 5V logic rail. One 100 nF cap across the breadboard +5V/GND power rails decouples this and prevents corrupt packets |

**Place the 100 nF cap as close as possible to the +5V/GND connection point on the breadboard rail.**

---

## Breadboard Wiring Diagram (ASCII)

```
SideWinder                  Breadboard              Nucleo CN8 / CN6
FFB Wheel
DB-15 Female
                            ┌──────────┐
 Pin 1  (+5V) ──────────────┤ +5V rail ├──────────── CN6 pin 5 (+5V)
 Pin 4  (GND) ──────────────┤ GND rail ├──────────── CN6 pin 7 (GND)
 Shell  (GND) ──────────────┤ GND rail │ 100nF ┐
                            └──────────┘  cap   ┘ (across rails)

 Pin 2  (CLK)  ─────────────────────────────────── CN8 pin 1 (A0 = PC0)
 Pin 7  (DATA0)─────────────────────────────────── CN8 pin 2 (A1 = PC1)
 Pin 3  (TRIG) ─────────────────────────────────── CN8 pin 3 (A2 = PC2)
 Pin 10 (DATA1)─────────────────────────────────── CN8 pin 4 (A3 = PC3)
 Pin 14 (DATA2)─────────────────────────────────── CN8 pin 5 (A4 = PC4)

 Pin 12 (MIDI) ─── [tape off / future Phase 4] ─── LPUART1 TX (TBD)
 Pin 11 (AX2)  ─── [leave unconnected]
```

---

## Protocol Summary

```
Necroware / Linux driver logic (C translation):

1. Nucleo holds TRIGGER (PC2/A2) LOW for 3 ms — packet cooldown
2. Nucleo pulses TRIGGER HIGH for 20 µs then LOW → wheel wakes up
3. Wheel starts self-clocked transmission:
     - CLOCK (Pin 2) goes HIGH → Nucleo waits for it
     - On each CLOCK RISING edge → sample DATA0, DATA1, DATA2 simultaneously
     - Each sample = 1 or 3 bits depending on mode
     - Stop when CLOCK stays LOW longer than timeout
4. Identify model by bit count:
     - 11 bits → FFB Wheel (3-bit mode)  ← your device
     - 33 bits → FFB Wheel (1-bit mode)  ← also your device
     - 15 bits → GamePad
     - 64 bits → 3D Pro
5. Decode 33 effective bits:
     bits  0–9   Steering  (10-bit, LSB first, 0–1023)
     bits 10–15  Brake     (6-bit, 0–63)
     bits 16–21  Gas       (6-bit, 0–63)
     bits 22–29  Buttons   (8-bit, active LOW → firmware inverts)
     bits 30–32  Parity    (XOR of all 33 bits must equal 0)
```

---

## Nucleo-G474RE Header Reference (MB1367-G474RE-C04)

```
CN8 (Arduino Analog — left side of board)        CN6 (Power)
┌──────────────┐                                 ┌──────────┐
│ 1 A0  A1  2  │   ← SideWinder CLK / DATA0      │ 1  NC    │
│ 3 A2  A3  4  │   ← SideWinder TRIGGER / DATA1  │ 2  IOREF │
│ 5 A4  A5  6  │   ← SideWinder DATA2             │ 3  NRST  │
└──────────────┘                                 │ 4  3.3V  │
                                                 │ 5  +5V ──┼── SideWinder Pin 1
                                                 │ 6  GND   │
                                                 │ 7  GND ──┼── SideWinder Pin 4 + Shell
                                                 │ 8  NC    │
                                                 └──────────┘
```

---

## Phase 4: Force Feedback MIDI Wiring (Future)

The Force Feedback motor commands travel from PC → wheel over MIDI protocol on **Gameport Pin 12**.  
This is a **31.25 kbaud UART** signal from the Nucleo to the wheel.  
UART assignment: **LPUART1** on **PC10** (TX) — available on CN10 pin 6 (D35).  
Do NOT use PA2 (USART2 TX — that is the debug serial port).

| Signal | Gameport Pin | Nucleo | STM32 |
|:---|:---:|:---:|:---:|
| MIDI OUT (FF commands) | Pin 12 | CN10 pin 6 (D35) | PC10 (LPUART1 TX) |

*Tape off Pin 12 for now. A 220 Ω series resistor will be required for the MIDI current loop when Phase 4 is implemented.*

---

*Last updated: 2026-06-18*  
*Protocol reference: [necroware/gameport-adapter](https://github.com/necroware/gameport-adapter) (GPL-3.0)*  
*Hardware reference: [STM32G4 RM0440](https://www.st.com/resource/en/reference_manual/rm0440.pdf) + NUCLEO-G474RE schematic MB1367*
