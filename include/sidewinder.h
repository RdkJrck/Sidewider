#ifndef __SIDEWINDER_H
#define __SIDEWINDER_H

#include "main.h"

/*
 * SideWinder Force Feedback Wheel — Gameport → Nucleo-G474RE GPIO mapping
 * Based on necroware/gameport-adapter Sidewinder.h + GamePort.h
 * Full reference: docs/WIRING.md
 *
 * Gameport  Signal        STM32   Nucleo   Direction
 * ──────────────────────────────────────────────────
 * Pin  1    +5V logic     —       CN6-5    Power
 * Pin  2    CLOCK (in)    PC0     A0 CN8   Input pull-up
 * Pin  3    TRIGGER (out) PC2     A2 CN8   Output PP
 * Pin  4    GND           —       CN6-7    Power
 * Pin  7    DATA 0 (in)   PC1     A1 CN8   Input pull-up
 * Pin 10    DATA 1 (in)   PC3     A3 CN8   Input pull-up
 * Pin 12    MIDI TX       —       (Phase4) —
 * Pin 14    DATA 2 (in)   PC4     A4 CN8   Input pull-up
 * Shell     Chassis GND   —       CN6-7    Power
 */

/* ── Pin definitions ──────────────────────────────────────────── */
#define SW_CLK_PORT      GPIOA
#define SW_CLK_PIN       GPIO_PIN_0   /* Gameport Pin 2  — CLOCK (Nucleo A0) */

#define SW_DATA0_PORT    GPIOA
#define SW_DATA0_PIN     GPIO_PIN_1   /* Gameport Pin 7  — DATA 0 (Nucleo A1) */

#define SW_DATA1_PORT    GPIOB
#define SW_DATA1_PIN     GPIO_PIN_0   /* Gameport Pin 10 — DATA 1 (Nucleo A3) */

#define SW_DATA2_PORT    GPIOC
#define SW_DATA2_PIN     GPIO_PIN_1   /* Gameport Pin 14 — DATA 2 (Nucleo A4) */

#define SW_STRB_PORT     GPIOA
#define SW_STRB_PIN      GPIO_PIN_5   /* Gameport Pin 3  — TRIGGER (Nucleo D13/LED) */

/* ── Protocol timing ──────────────────────────────────────────── */
#define SW_TRIGGER_US    20u          /* Trigger pulse width (µs) */
#define SW_COOLDOWN_MS    3u          /* Min gap between packets (ms) */
#define SW_WAIT_LOOPS   200u          /* Edge-wait loop iterations */
#define SW_MAX_BITS     128u          /* Max bits per packet */

/* ── Model identifiers (mirrors necroware guessModel) ─────────── */
typedef enum {
  SW_MODEL_UNKNOWN  = 0,
  SW_MODEL_GAMEPAD  = 1,   /* 15 bits */
  SW_MODEL_3D_PRO   = 2,   /* 64 bits */
  SW_MODEL_FFB_PRO  = 3,   /* 16 or 48 bits */
  SW_MODEL_FFB_WHEEL = 4   /* 11 or 33 bits ← your device */
} SW_Model_t;

/* ── Parsed state ─────────────────────────────────────────────── */
typedef struct {
  uint16_t steering;    /* 10-bit: 0–1023  (bits 0–9) */
  uint8_t  brake;       /* 6-bit:  0–63    (bits 10–15) */
  uint8_t  gas;         /* 6-bit:  0–63    (bits 16–21) */
  uint8_t  buttons;     /* 8-bit bitmask, active-HIGH (bits 22–29, inverted) */
  uint8_t  valid;       /* 1 = parity OK + model matched */
  SW_Model_t model;
  uint8_t  packet_bits; /* Raw bit count received */
} SideWinder_Data_t;

/* ── Public API ───────────────────────────────────────────────── */
void       SideWinder_Init(void);
void       SideWinder_Wakeup(void);
SW_Model_t SideWinder_Identify(void);
uint8_t    SideWinder_Read(SideWinder_Data_t *data);

#endif /* __SIDEWINDER_H */
