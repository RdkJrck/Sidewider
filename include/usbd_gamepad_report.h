#ifndef __USBD_GAMEPAD_REPORT_H
#define __USBD_GAMEPAD_REPORT_H

#include <stdint.h>

/* Must match the size of Gamepad_Report_t (7 bytes, padded to 8) */
#define HID_EPIN_ADDR   0x81U
#define HID_EPIN_SIZE   0x08U
#define HID_FS_BINTERVAL 0x0AU
#define USB_HID_CONFIG_DESC_SIZ  34U
#define USB_HID_DESC_SIZ         9U
#define HID_REPORT_DESC          0x22U
#define HID_DESCRIPTOR_TYPE      0x21U

/* HID Gamepad Report Structure */
typedef struct __attribute__((packed))
{
  int16_t steering;    /* Steering: -32767 to 32767 */
  uint16_t accelerator; /* Accelerator: 0 to 65535 */
  uint16_t brake;       /* Brake: 0 to 65535 */
  uint8_t buttons;      /* 8 Buttons (Bitmask) */
} Gamepad_Report_t;

/* HID Gamepad Report Descriptor 
 * Configured for a Racing Wheel with Pedals.
 */
static const uint8_t HID_GAMEPAD_ReportDesc[] =
{
    0x05, 0x01,        /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x04,        /* USAGE (Joystick) */
    0xa1, 0x01,        /* COLLECTION (Application) */
    
    /* Steering Axis */
    0x05, 0x01,        /*   USAGE_PAGE (Generic Desktop) */
    0x09, 0x30,        /*   USAGE (X - Steering) */
    0x16, 0x01, 0x80,  /*   LOGICAL_MINIMUM (-32767) */
    0x26, 0xff, 0x7f,  /*   LOGICAL_MAXIMUM (32767) */
    0x75, 0x10,        /*   REPORT_SIZE (16) bits */
    0x95, 0x01,        /*   REPORT_COUNT (1) */
    0x81, 0x02,        /*   INPUT (Data,Var,Abs) */

    /* Pedals - Using Simulation Controls for better game detection */
    0x05, 0x02,        /*   USAGE_PAGE (Simulation Controls) */
    0x09, 0xc4,        /*   USAGE (Accelerator) */
    0x09, 0xc5,        /*   USAGE (Brake) */
    0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
    0x26, 0xff, 0xff,  /*   LOGICAL_MAXIMUM (65535) */
    0x75, 0x10,        /*   REPORT_SIZE (16) bits */
    0x95, 0x02,        /*   REPORT_COUNT (2) */
    0x81, 0x02,        /*   INPUT (Data,Var,Abs) */

    /* 8 Buttons */
    0x05, 0x09,        /*   USAGE_PAGE (Buttons) */
    0x19, 0x01,        /*   USAGE_MINIMUM (Button 1) */
    0x29, 0x08,        /*   USAGE_MAXIMUM (Button 8) */
    0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01,        /*   LOGICAL_MAXIMUM (1) */
    0x75, 0x01,        /*   REPORT_SIZE (1) bit */
    0x95, 0x08,        /*   REPORT_COUNT (8) buttons */
    0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
    
    0xc0               /* END_COLLECTION */
};

#endif /* __USBD_GAMEPAD_REPORT_H */
