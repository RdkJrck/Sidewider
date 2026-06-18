#include "main.h"
#include "sidewinder.h"
#include "usb_device.h"
#include "usbd_gamepad_report.h"
#include <ctype.h>
#include <stdio.h>

/* Human-readable model name for UART debug is now handled inline in main loop */

UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void Error_Handler(void);
extern uint8_t USBD_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report,
                                   uint16_t len);
extern USBD_HandleTypeDef hUsbDeviceFS;

/**
 * @brief  Process interactive UART command (e.g. "PA5 1" or "PB3 R")
 */
void Process_CLI_Command(char *cmd) {
  char port_char;
  int pin_num;
  char state_char;

  if (sscanf(cmd, "P%c%d %c", &port_char, &pin_num, &state_char) == 3) {
    port_char = toupper(port_char);
    if (port_char < 'A' || port_char > 'G' || pin_num < 0 || pin_num > 15) {
      printf("Invalid Port/Pin: %s\r\n", cmd);
      return;
    }

    GPIO_TypeDef *port = NULL;
    switch (port_char) {
    case 'A':
      port = GPIOA;
      __HAL_RCC_GPIOA_CLK_ENABLE();
      break;
    case 'B':
      port = GPIOB;
      __HAL_RCC_GPIOB_CLK_ENABLE();
      break;
    case 'C':
      port = GPIOC;
      __HAL_RCC_GPIOC_CLK_ENABLE();
      break;
    case 'D':
      port = GPIOD;
      __HAL_RCC_GPIOD_CLK_ENABLE();
      break;
    case 'E':
      port = GPIOE;
      __HAL_RCC_GPIOE_CLK_ENABLE();
      break;
    case 'F':
      port = GPIOF;
      __HAL_RCC_GPIOF_CLK_ENABLE();
      break;
    case 'G':
      port = GPIOG;
      __HAL_RCC_GPIOG_CLK_ENABLE();
      break;
    }

    uint16_t pin = (1 << pin_num);
    GPIO_InitTypeDef init = {0};
    init.Pin = pin;

    state_char = toupper(state_char);
    if (state_char == 'R') {
      init.Mode = GPIO_MODE_INPUT;
      init.Pull = GPIO_PULLDOWN;
      HAL_GPIO_Init(port, &init);
      GPIO_PinState st = HAL_GPIO_ReadPin(port, pin);
      printf("Pin P%c%d is %s\r\n", port_char, pin_num,
             st == GPIO_PIN_SET ? "HIGH" : "LOW");
    } else if (state_char == '0' || state_char == '1') {
      init.Mode = GPIO_MODE_OUTPUT_PP;
      init.Pull = GPIO_NOPULL;
      init.Speed = GPIO_SPEED_FREQ_LOW;
      HAL_GPIO_Init(port, &init);
      HAL_GPIO_WritePin(port, pin,
                        state_char == '1' ? GPIO_PIN_SET : GPIO_PIN_RESET);
      printf("Set P%c%d to %c\r\n", port_char, pin_num, state_char);
    } else {
      printf("Invalid state. Use 0, 1, or R\r\n");
    }
  } else {
    printf("Syntax: PA5 1  or  PB3 R\r\n");
  }
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Initialize GPIO early for status signaling */
  MX_GPIO_Init();

  /* Power on LED to signal life - 3 obvious blinks (at HSI 16MHz before clock
   * switch) */
  for (int j = 0; j < 3; j++) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(300);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(300);
  }

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize UART for Debugging */
  MX_USART2_UART_Init();
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("\r\n\r\n=== SideWinder Debug UART Active ===\r\n> ");

  /* Checkpoint 1: Clock configured — 1 long blink (clearly visible) */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_Delay(200);

  /* Checkpoint 2: Ready for USB — 2 fast blinks */
  for (int j = 0; j < 2; j++) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(100);
  }

  /* Initialize USB Stack (HAL_PCD_Start inside sets DPPU automatically) */
  MX_USB_Device_Init();

  /* --- NUCLEO SELF-TEST --- */
  printf("\r\n\r\n======================================================\r\n");
  printf("[SELF-TEST] Please plug a jumper wire directly from D13 to A0.\r\n");
  printf("[SELF-TEST] We are testing your Nucleo pins right now...\r\n");
  
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  GPIO_InitTypeDef gtest = {0};
  gtest.Pin = GPIO_PIN_5; /* D13 / PA5 */
  gtest.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOA, &gtest);
  
  gtest.Pin = GPIO_PIN_0; /* A0 / PA0 */
  gtest.Mode = GPIO_MODE_INPUT;
  gtest.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &gtest);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  HAL_Delay(50);
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(50);
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
       printf("[SELF-TEST] ---> SUCCESS! The pins and wire work perfectly!\r\n");
    } else {
       printf("[SELF-TEST] ---> FAILED! Pin stuck HIGH.\r\n");
    }
  } else {
    printf("[SELF-TEST] ---> FAILED! Pin didn't go HIGH. Wire broken or wrong pins.\r\n");
  }
  printf("======================================================\r\n\r\n");
  HAL_Delay(5000); /* Wait 5 seconds so user can read it */

  /* Initialize SideWinder Driver */
  SideWinder_Init();

  /* Boot diagnostic — runs once, prints full pin + packet analysis */
  printf("\r\n[DIAG] Checking USB: look for 0483:5710 in lsusb on the PC\r\n");
  printf("[DIAG] Checking SideWinder pins (idle state — all should be HIGH):\r\n");
  printf("  CLK   PA0/A0 (SW pin 2):  %s\r\n",
         (GPIOA->IDR & GPIO_PIN_0) ? "HIGH ok" : "LOW  <- check wire");
  printf("  DATA0 PA1/A1 (SW pin 7):  %s\r\n",
         (GPIOA->IDR & GPIO_PIN_1) ? "HIGH ok" : "LOW  <- check wire");
  printf("  DATA1 PB0/A3 (SW pin 10): %s\r\n",
         (GPIOB->IDR & GPIO_PIN_0) ? "HIGH ok" : "LOW  <- check wire");
  printf("  DATA2 PC1/A4 (SW pin 14): %s\r\n",
         (GPIOC->IDR & GPIO_PIN_1) ? "HIGH ok" : "LOW  <- check wire");

  printf("[DIAG] Sending trigger, watching CLK for activity...\r\n");
  /* cooldown then single trigger */
  SideWinder_Wakeup();
  /* count CLK transitions in next 2ms */
  uint32_t trans = 0;
  uint8_t last_clk = (GPIOA->IDR & GPIO_PIN_0) ? 1 : 0;
  uint32_t t0 = HAL_GetTick();
  while (HAL_GetTick() - t0 < 2) {
    uint8_t cur = (GPIOA->IDR & GPIO_PIN_0) ? 1 : 0;
    if (cur != last_clk) { trans++; last_clk = cur; }
  }
  if (trans == 0)
    printf("  CLK: NO transitions -> wheel not responding (check power/wiring)\r\n");
  else
    printf("  CLK: %lu transitions -> wheel is alive!\r\n", trans);

  printf("[DIAG] Full packet read:\r\n");
  SW_Model_t bootModel = SideWinder_Identify();
  (void)bootModel;

  /* Gamepad state */
  Gamepad_Report_t myGamepad = {0};
  SideWinder_Data_t swData = {0};

  /* Infinite loop */
  char rx_buf[32];
  int rx_idx = 0;

  while (1) {
    /* Clear any UART Errors that lock up RX */
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE | UART_FLAG_NE |
                                         UART_FLAG_FE | UART_FLAG_PE)) {
      __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF |
                                         UART_CLEAR_FEF | UART_CLEAR_PEF);
    }

    /* Read UART for CLI using raw register to bypass HAL lockups */
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
      uint8_t c = (uint8_t)(huart2.Instance->RDR & 0xFF);

      /* Echo character */
      HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY);

      if (c == '\r' || c == '\n') {
        printf("\n");
        rx_buf[rx_idx] = '\0';
        if (rx_idx > 0) {
          Process_CLI_Command(rx_buf);
        }
        rx_idx = 0;
        printf("> ");
      } else if (c == '\b' || c == 0x7F) {
        if (rx_idx > 0)
          rx_idx--;
      } else if (rx_idx < 31) {
        rx_buf[rx_idx++] = c;
      }
    }
    /* ── SideWinder poll + USB report every 10ms ── */
    static uint32_t lastReport = 0;
    if (HAL_GetTick() - lastReport >= 10) {
      SideWinder_Read(&swData);

      if (swData.valid) {
        /* Steering: 10-bit (0-1023) → 16-bit signed (-32767…32767) */
        myGamepad.steering = (int16_t)(((int32_t)swData.steering - 512) * 64);
        /* Gas / Brake: 6-bit (0-63) -> inverted so 0 is idle, 63 is pressed -> 16-bit unsigned */
        myGamepad.accelerator = (uint16_t)((63 - swData.gas) * 1040);
        myGamepad.brake = (uint16_t)((63 - swData.brake) * 1040);
        myGamepad.buttons = swData.buttons;
        USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t *)&myGamepad,
                            sizeof(myGamepad));
      }

      lastReport = HAL_GetTick();
    }

    /* ── UART diagnostic dump every 5000ms ── */
    static uint32_t lastDump = 0;
    static uint16_t dumpCount = 0;
    if (HAL_GetTick() - lastDump >= 5000) {
      dumpCount++;
      const char *mname;
      switch (swData.model) {
        case SW_MODEL_FFB_WHEEL: mname = "FFB_Wheel";  break;
        case SW_MODEL_GAMEPAD:   mname = "GamePad";    break;
        case SW_MODEL_3D_PRO:    mname = "3D_Pro";     break;
        case SW_MODEL_FFB_PRO:   mname = "FFB_Pro";    break;
        default:                 mname = "Unknown";    break;
      }
      printf("[%4u] %-10s pkt=%2u | ", dumpCount, mname, swData.packet_bits);
      if (swData.valid) {
        printf("steer=%4u gas=%3u brk=%3u btn=0x%02X\r\n",
               swData.steering, swData.gas, swData.brake, swData.buttons);
      } else if (swData.packet_bits == 0) {
        printf("NO RESPONSE -- check wiring/power\r\n");
      } else {
        printf("pkt=%u bits (wrong size or bad parity)\r\n", swData.packet_bits);
      }
      lastDump = HAL_GetTick();
    }

    /* Heartbeat: slow blink every 1s to confirm main loop is alive */
    static uint32_t lastHeartbeat = 0;
    if (HAL_GetTick() - lastHeartbeat >= 1000) {
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
      lastHeartbeat = HAL_GetTick();
    }
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /* Enable PWR clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* SYSCLK: HSI 16 MHz (internal oscillator, always available) → PLL → 144 MHz
   * NOTE:   HSE crystal X3 on MB1367C requires SB25+SB26 to be bridged — NOT
   * done by default. Using HSI avoids that hardware dependency entirely. USB:
   * HSI48 trimmed by CRS (designed for USB, ±2500 ppm → well within USB spec)
   * PLL math: 16 / 4 = 4 MHz VCO in → × 72 = 288 MHz VCO → / 2 = 144 MHz SYSCLK
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON; /* needed for USB clock */
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4; /* 16 / 4 = 4 MHz */
  RCC_OscInitStruct.PLL.PLLN = 72;            /* 4 * 72 = 288 MHz VCO */
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV6;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2; /* 288 / 2 = 144 MHz SYSCLK */
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /* Initialize CPU, AHB and APB bus clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }

  /* USB uses HSI48, trimmed by CRS to lock precisely to USB SOF signal.
   * This is the ST-recommended approach for USB without external crystal on USB
   * path. */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }

  /* Enable CRS and sync HSI48 to USB SOF for precision trimming */
  __HAL_RCC_CRS_CLK_ENABLE();
  HAL_RCCEx_CRSConfig(&(RCC_CRSInitTypeDef){
      .Prescaler = RCC_CRS_SYNC_DIV1,
      .Source = RCC_CRS_SYNC_SOURCE_USB,
      .Polarity = RCC_CRS_SYNC_POLARITY_RISING,
      .ReloadValue = RCC_CRS_RELOADVALUE_DEFAULT,
      .ErrorLimitValue = RCC_CRS_ERRORLIMIT_DEFAULT,
      .HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT,
  });
}

/**
 * @brief USART2 Initialization Function
 */
static void MX_USART2_UART_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Enable Peripheral Clocks */
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USART2 GPIO Configuration: PA2 -> TX, PA3 -> RX */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Configure USART2 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief Retargets the C library printf function to the USART2.
 */
int _write(int file, char *ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /* Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* Rapid blink indicates error state */
  while (1) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    for (volatile int i = 0; i < 100000; i++)
      ;
  }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
}
#endif /* USE_FULL_ASSERT */