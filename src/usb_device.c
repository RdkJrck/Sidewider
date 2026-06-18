#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid_gamepad.h"
#include "main.h"
#include <stdio.h>

/* USB Device Core handle declaration */
USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_Device_Init(void)
{
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, 0) != USBD_OK)
  {
    printf("USB: USBD_Init FAILED\r\n");
    Error_Handler();
  }
  printf("USB: USBD_Init OK\r\n");

  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_HID_Gamepad) != USBD_OK)
  {
    printf("USB: RegisterClass FAILED\r\n");
    Error_Handler();
  }
  printf("USB: RegisterClass OK\r\n");

  if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
  {
    printf("USB: USBD_Start FAILED\r\n");
    Error_Handler();
  }
  printf("USB: USBD_Start OK\r\n");

  /* Force D+ pull-up explicitly in case HAL_PCD_Start didn't set it */
  HAL_Delay(100);
  USB->BCDR |= USB_BCDR_DPPU;
  printf("USB: DPPU forced HIGH — device should now appear on host\r\n");
}
