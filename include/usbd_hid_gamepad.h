#ifndef __USBD_HID_GAMEPAD_H
#define __USBD_HID_GAMEPAD_H

#include "usbd_ioreq.h"

/* HID Gamepad Report Descriptor Size */
#define USBD_HID_REPORT_DESC_SIZE     74

/* USB HID Class handle */
extern USBD_ClassTypeDef  USBD_HID_Gamepad;

/* HID Gamepad Specific Function Prototypes */
uint8_t USBD_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len);

#endif /* __USBD_HID_GAMEPAD_H */
