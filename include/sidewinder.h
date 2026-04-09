#ifndef __SIDEWINDER_H
#define __SIDEWINDER_H

#include "main.h"

/* Port and Pin Definitions (Matching A0, A1, A2 on Nucleo-G474RE) */
#define SW_PORT      GPIOC
#define SW_DATA_PIN  GPIO_PIN_0  /* A0 */
#define SW_CLK_PIN   GPIO_PIN_1  /* A1 */
#define SW_STRB_PIN  GPIO_PIN_2  /* A2 */

typedef struct {
  uint16_t steering;  /* 10-bit: 0-1023 */
  uint8_t  gas;       /* 6-bit:  0-63   */
  uint8_t  brake;     /* 6-bit:  0-63   */
  uint8_t  buttons;   /* 8 buttons      */
  uint8_t  valid;     /* 1 if parity is OK */
} SideWinder_Data_t;

void SideWinder_Init(void);
void SideWinder_Read(SideWinder_Data_t *data);

#endif /* __SIDEWINDER_H */
