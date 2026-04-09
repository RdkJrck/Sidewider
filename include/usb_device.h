#ifndef __USB_DEVICE_H
#define __USB_DEVICE_H

#include "stm32g4xx_hal.h"
#include "usbd_core.h"
#include "usbd_def.h"
#include "usbd_hid.h"

/* USB Device initialization function */
void MX_USB_Device_Init(void);

#endif /* __USB_DEVICE_H */
