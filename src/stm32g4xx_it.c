#include "stm32g4xx_hal.h"
#include "main.h"
#include <stdio.h>

void SysTick_Handler(void) { HAL_IncTick(); }

extern UART_HandleTypeDef huart2;

void HardFault_Handler(void) {
  /* Read the program counter at the moment of the fault from the stack */
  __asm volatile (
    "TST LR, #4      \n"
    "ITE EQ          \n"
    "MRSEQ R0, MSP   \n"
    "MRSNE R0, PSP   \n"
    "B HardFault_Print \n"
  );
}

void HardFault_Print(uint32_t *stack) {
  printf("\r\n!!! HARDFAULT !!!\r\n");
  printf("PC  = 0x%08lX\r\n", stack[6]); /* Program Counter */
  printf("LR  = 0x%08lX\r\n", stack[5]); /* Link Register */
  printf("R0  = 0x%08lX\r\n", stack[0]);
  while (1) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    HAL_Delay(50);
  }
}