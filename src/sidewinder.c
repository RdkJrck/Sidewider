#include "sidewinder.h"

/* Helper for short delay (~1 microsecond) */
static void delay_us(uint32_t us) {
  /* At 170MHz, roughly 170 cycles per us. Simple loop for now. */
  for (volatile uint32_t i = 0; i < us * 20; i++);
}

void SideWinder_Init(void) {
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* PC0 (Data) and PC1 (Clock) as Inputs */
  GPIO_InitStruct.Pin = SW_DATA_PIN | SW_CLK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SW_PORT, &GPIO_InitStruct);

  /* PC2 (Strobe) as Output */
  GPIO_InitStruct.Pin = SW_STRB_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SW_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(SW_PORT, SW_STRB_PIN, GPIO_PIN_RESET);
}

void SideWinder_Read(SideWinder_Data_t *data) {
  uint8_t raw_bits[64] = {0};
  uint8_t count = 0;
  uint32_t timeout;

  /* Reset data structure */
  data->steering = 0;
  data->gas = 0;
  data->brake = 0;
  data->buttons = 0;
  data->valid = 0;

  /* Disable interrupts removed: cannot block USB IRQ which has a strict timeout! */
  // uint32_t primask = __get_PRIMASK();
  // __disable_irq();

  /* 1. Send Strobe Pulse (20-30us) */
  SW_PORT->BSRR = SW_STRB_PIN; /* High */
  delay_us(25);
  SW_PORT->BRR = SW_STRB_PIN;  /* Low */

  /* 2. Wait for first Clock pulse (Initial high) to start */
  timeout = 10000;
  while (!(SW_PORT->IDR & SW_CLK_PIN) && --timeout);
  if (timeout == 0) goto end;

  /* 3. Bit Capture Loop (Expecting ~33 bits for FFB Wheel) */
  while (count < 64) {
    /* Wait for Falling Edge of Clock */
    timeout = 10000;
    while ((SW_PORT->IDR & SW_CLK_PIN) && --timeout);
    if (timeout == 0) break;

    /* Read Data bit on falling edge (Sidewinder standard) */
    raw_bits[count++] = (SW_PORT->IDR & SW_DATA_PIN) ? 1 : 0;

    /* Wait for Rising Edge of Clock or Timeout */
    timeout = 10000;
    while (!(SW_PORT->IDR & SW_CLK_PIN) && --timeout);
    if (timeout == 0) break;
  }

  /* 4. Bit Parsing (FFB Wheel Mapping) */
  if (count >= 33) {
    /* Steering: Bits 0-9 */
    for (int i = 0; i < 10; i++) {
        data->steering |= (raw_bits[i] << i);
    }
    /* Brake: Bits 10-15 */
    for (int i = 0; i < 6; i++) {
        data->brake |= (raw_bits[10 + i] << i);
    }
    /* Gas: Bits 16-21 */
    for (int i = 0; i < 6; i++) {
        data->gas |= (raw_bits[16 + i] << i);
    }
    /* Buttons: Bits 22-29 */
    for (int i = 0; i < 8; i++) {
        data->buttons |= (raw_bits[22 + i] << i);
    }
    data->valid = 1; /* Basic check, could add parity here later */
  }

end:
  /* Restore interrupts removed: we no longer touch interrupts here */
  // __set_PRIMASK(primask);
}
