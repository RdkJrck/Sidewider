#include "main.h"
#include "usb_device.h"
#include "usbd_gamepad_report.h"
#include "sidewinder.h"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void Error_Handler(void);
extern uint8_t USBD_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len);
extern USBD_HandleTypeDef hUsbDeviceFS;

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Initialize GPIO early for status signaling */
  MX_GPIO_Init();

  /* Power on LED to signal life - 3 obvious blinks */
  for(int j=0; j<3; j++) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    for(volatile int i=0; i<2000000; i++); 
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    for(volatile int i=0; i<2000000; i++); 
  }

  /* Configure the system clock */
  SystemClock_Config();
  
  /* Checkpoint 1: Clock configured */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  for(volatile int i=0; i<200000; i++); 
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /* Initialize USB Stack */
  MX_USB_Device_Init();

  /* Force DP pull-up to notify PC manually */
  USB->BCDR |= (1U << 15U);
  
  /* Checkpoint 2: USB Initialized */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  for(volatile int i=0; i<200000; i++); 
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /* Initialize SideWinder Driver */
  SideWinder_Init();

  /* Gamepad state */
  Gamepad_Report_t myGamepad = {0};
  SideWinder_Data_t swData = {0};

  /* Infinite loop */
  while (1)
  {
    /* Heartbeat removed for diagnostic */
    /* Send USB Gamepad report every 10ms */
    static uint32_t lastReport = 0;
    if (HAL_GetTick() - lastReport >= 10) {
      /* 1. Read from SideWinder Wheel */
      SideWinder_Read(&swData);

      if (swData.valid) {
        /* 2. Map SideWinder bits to HID Gamepad axes/buttons */
        /* Steering: 10-bit (0-1023) -> 16-bit signed (-32767 to 32767) */
        myGamepad.steering = (int16_t)(( (int32_t)swData.steering - 512 ) * 64);
        
        /* Accelerator/Brake: 6-bit (0-63) -> 16-bit unsigned (0 to 65535) */
        myGamepad.accelerator = (uint16_t)(swData.gas * 1040);
        myGamepad.brake = (uint16_t)(swData.brake * 1040);
        
        /* Buttons: Direct bitmask mapping */
        myGamepad.buttons = swData.buttons;

        /* 3. Send over USB */
        USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&myGamepad, sizeof(myGamepad));
      }
      
      lastReport = HAL_GetTick();
    }
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* Enable PWR clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure USB Clock source */
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable CRS clock */
  __HAL_RCC_CRS_CLK_ENABLE();
  /* Configure CRS to sync HSI48 with USB SOF */
  HAL_RCCEx_CRSConfig(&(RCC_CRSInitTypeDef){
    .Prescaler = RCC_CRS_SYNC_DIV1,
    .Source = RCC_CRS_SYNC_SOURCE_USB,
    .Polarity = RCC_CRS_SYNC_POLARITY_RISING,
    .ReloadValue = RCC_CRS_RELOADVALUE_DEFAULT,
    .ErrorLimitValue = RCC_CRS_ERRORLIMIT_DEFAULT,
    .HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT
  });
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
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
void Error_Handler(void)
{
  /* Rapid blink indicates error state */
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    for (volatile int i = 0; i < 100000; i++); 
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */