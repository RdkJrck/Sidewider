#include "stm32g4xx_hal.h"
#include "main.h"

void SysTick_Handler(void) { HAL_IncTick(); }