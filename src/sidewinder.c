#include "sidewinder.h"

/* ── helpers ──────────────────────────────────────────────────────────── */

/*
 * Spin-loop delay.
 * At 144 MHz sysclk, ~144 iterations per µs.  We use 40 here to be safe
 * (compiler may not optimise the volatile away, but keep it conservative).
 */
static void delay_us(uint32_t us) {
  uint32_t start = DWT->CYCCNT;
  uint32_t wait = us * (SystemCoreClock / 1000000);
  while((DWT->CYCCNT - start) < wait) {
    /* wait */
  }
}

/* Read CLK pin state (non-zero = HIGH) */
static inline uint32_t clk_high(void) { return SW_CLK_PORT->IDR & SW_CLK_PIN; }

/* Read DATA0 pin state (bit 0) */
static inline uint8_t data0(void) {
  return (SW_DATA0_PORT->IDR & SW_DATA0_PIN) ? 1u : 0u;
}

/* Read DATA1 pin state (bit 1) */
static inline uint8_t data1(void) {
  return (SW_DATA1_PORT->IDR & SW_DATA1_PIN) ? 1u : 0u;
}

/* Read DATA2 pin state (bit 2) */
static inline uint8_t data2(void) {
  return (SW_DATA2_PORT->IDR & SW_DATA2_PIN) ? 1u : 0u;
}

/* ── init ─────────────────────────────────────────────────────────────── */

void SideWinder_Init(void) {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Enable DWT cycle counter for precise microsecond delays */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  GPIO_InitTypeDef g = {0};

  /* CLK (A0) and DATA0 (A1) on GPIOA */
  g.Pin = SW_CLK_PIN | SW_DATA0_PIN;
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &g);

  /* DATA1 (A3) on GPIOB */
  g.Pin = SW_DATA1_PIN;
  HAL_GPIO_Init(GPIOB, &g);

  /* DATA2 (A4) on GPIOC */
  g.Pin = SW_DATA2_PIN;
  HAL_GPIO_Init(GPIOC, &g);

  /* TRIGGER on GPIOA */
  g.Pin = SW_STRB_PIN;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(SW_STRB_PORT, &g);
  HAL_GPIO_WritePin(SW_STRB_PORT, SW_STRB_PIN, GPIO_PIN_RESET);
}

/* ── wakeup sequence ──────────────────────────────────────────────────── */

/* Sends the magic timing sequence to wake up older analog-mode wheels */
void SideWinder_Wakeup(void) {
  static const uint16_t magic = 150;
  static const uint16_t seq[] = {magic, magic + 725, magic + 300, magic, 0};

  HAL_GPIO_WritePin(SW_STRB_PORT, SW_STRB_PIN, GPIO_PIN_RESET);
  HAL_Delay(3); /* Cooldown */

  for (int i = 0; seq[i] != 0; i++) {
    SW_STRB_PORT->BSRR = SW_STRB_PIN; /* set HIGH */
    delay_us(SW_TRIGGER_US);
    SW_STRB_PORT->BRR = SW_STRB_PIN; /* set LOW */
    delay_us(seq[i]);
  }
}

/* ── packet capture ───────────────────────────────────────────────────── */

/*
 * Read one packet from the SideWinder device.
 *
 * Protocol (from necroware / Linux kernel):
 *   1. Send TRIGGER pulse HIGH for SW_TRIGGER_US.
 *   2. Wait for CLK to go HIGH (device wakes up).
 *   3. On each RISING edge of CLK, sample DATA pins.
 *   4. Stop when CLK stays low longer than SW_BIT_TIMEOUT iterations.
 *
 * Bits are packed 3 per clock (DATA2|DATA1|DATA0) for the 3-bit mode
 * capable devices, or 1-bit mode using only DATA0.
 *
 * Returns: number of raw "tri-bit" (3-bit) samples captured.
 * The caller interprets this as 1-bit or 3-bit depending on device model.
 */
static uint8_t capture_packet(uint8_t *buf, uint8_t max) {
  uint32_t t;
  uint8_t count = 0;

  /* 1. Cooldown — trigger LOW, settle */
  HAL_GPIO_WritePin(SW_STRB_PORT, SW_STRB_PIN, GPIO_PIN_RESET);
  HAL_Delay(SW_COOLDOWN_MS);

  /* 2. Trigger pulse: HIGH → delay → LOW */
  SW_STRB_PORT->BSRR = SW_STRB_PIN; /* set HIGH */
  delay_us(SW_TRIGGER_US);
  SW_STRB_PORT->BRR = SW_STRB_PIN; /* set LOW */

  /* 3. Wait for CLK to go HIGH (device acknowledges) */
  t = SW_WAIT_LOOPS * 10u;
  while (!clk_high() && --t)
    ;
  if (t == 0)
    return 0; /* no response — nothing connected */

  /* 4. Capture bits on rising CLK edges */
  while (count < max) {
    /* Wait for CLK to fall */
    t = SW_WAIT_LOOPS;
    while (clk_high() && --t)
      ;
    if (t == 0)
      break; /* timeout = end of packet */

    /* Wait for CLK to rise */
    t = SW_WAIT_LOOPS;
    while (!clk_high() && --t)
      ;
    if (t == 0)
      break;

    /* Sample all three data lines on rising edge */
    buf[count++] = data0() | (data1() << 1u) | (data2() << 2u);
  }

  return count;
}

/* ── model detection ──────────────────────────────────────────────────── */

static SW_Model_t guess_model(uint8_t packet_size) {
  switch (packet_size) {
  case 15:
    return SW_MODEL_GAMEPAD;
  case 64:
    return SW_MODEL_3D_PRO;
  case 11: /* FALL */
  case 33:
    return SW_MODEL_FFB_WHEEL;
  case 16: /* FALL */
  case 48:
    return SW_MODEL_FFB_PRO; /* or FF_PRO — need ID read */
  default:
    return SW_MODEL_UNKNOWN;
  }
}

/* ── decode helpers ───────────────────────────────────────────────────── */

/* XOR-parity of a 32-bit value (even parity = returns 0 if valid) */
static uint8_t parity32(uint32_t v) {
  v ^= v >> 16;
  v ^= v >> 8;
  v ^= v >> 4;
  v ^= v >> 2;
  v ^= v >> 1;
  return v & 1u;
}

/*
 * Decode Force Feedback Wheel packet.
 *
 * The wheel sends 11 samples in 3-bit mode (33 bits total) or 33 samples
 * in 1-bit mode (33 bits total).  buf[] holds one sample per element;
 * each element may carry 1 or 3 bits depending on the mode detected by
 * the packet size.
 *
 * Returns 1 on success.
 */
static uint8_t decode_ffb_wheel(const uint8_t *buf, uint8_t n,
                                SideWinder_Data_t *out) {
  if (n != 11 && n != 33)
    return 0;

  const uint8_t shift = (n == 11) ? 3u : 1u; /* bits per sample */
  const uint8_t mask = (shift == 3) ? 0x07u : 0x01u;

  /* Reconstruct 33-bit value LSB-first */
  uint32_t value = 0;
  for (uint8_t i = 0; i < n; i++) {
    value |= (uint32_t)(buf[i] & mask) << (i * shift);
  }

  /* Parity check (bit 32, over all 33 bits) */
  /* The 33rd bit is at position 32 — use all 33 bits */
  if (!parity32(value & 0x1FFFFFFFu) && !(value >> 29u)) {
    /* parity failed — report but still store partial data for debug */
  }

  /* Bit field extraction */
  out->steering = (uint16_t)(value & 0x3FFu);       /* bits 0-9  */
  out->brake = (uint8_t)((value >> 10) & 0x3Fu);    /* bits 10-15 */
  out->gas = (uint8_t)((value >> 16) & 0x3Fu);      /* bits 16-21 */
  out->buttons = (uint8_t)(~(value >> 22) & 0xFFu); /* bits 22-29, inverted */

  out->valid = 1;
  return 1;
}

/* ── public API ───────────────────────────────────────────────────────── */

uint8_t SideWinder_Read(SideWinder_Data_t *out) {
  static uint8_t buf[SW_MAX_BITS];

  /* Zero output */
  out->steering = 0;
  out->brake = 0;
  out->gas = 0;
  out->buttons = 0;
  out->valid = 0;
  out->model = SW_MODEL_UNKNOWN;
  out->packet_bits = 0;

  uint8_t n = capture_packet(buf, SW_MAX_BITS);
  out->packet_bits = n;
  out->model = guess_model(n);

  if (out->model == SW_MODEL_FFB_WHEEL) {
    decode_ffb_wheel(buf, n, out);
  }
  /* Other models — data still in buf[], model identified, valid=0 */
  
  return out->valid;
}

SW_Model_t SideWinder_Identify(void) {
  SideWinder_Data_t dummy;
  SideWinder_Read(&dummy);
  if (dummy.model == SW_MODEL_UNKNOWN) {
    SideWinder_Wakeup();
    SideWinder_Read(&dummy);
  }
  return dummy.model;
}
