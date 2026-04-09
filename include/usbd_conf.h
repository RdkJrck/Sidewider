#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#include "stm32g4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

/* Workaround for wint_t error in system headers */
#ifndef _WINT_T
#define _WINT_T
typedef uint32_t wint_t;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* USB Device Library Configuration */
#define USBD_MAX_NUM_INTERFACES     1
#define USBD_MAX_NUM_CONFIGURATION   1
#define USBD_MAX_STR_DESC_SIZ       512
#define USBD_SUPPORT_USER_STRING_DESC 1
#define USBD_SELF_POWERED           1
#define USBD_DEBUG_LEVEL            0

/* HID Class Configuration */
#define USBD_HID_OUT_REPORT_COUNT   0

/* Memory Management */
#define USBD_malloc               malloc
#define USBD_free                 free
#define USBD_memset               memset
#define USBD_memcpy               memcpy

#endif /* __USBD_CONF_H */
