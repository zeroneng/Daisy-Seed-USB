/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @brief   USB Device configuration for Daisy Seed HID sample
  ******************************************************************************
  */
#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"

/* -------------------------------------------------------------------------- */
/* USB Device configuration                                                    */
/* -------------------------------------------------------------------------- */

/* This HID implementation uses a single USB interface. If the project later
   becomes composite, revisit this value to match the descriptor layout. */
#define USBD_MAX_NUM_INTERFACES     1U

#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       512U
#define USBD_SUPPORT_USER_STRING    1U

/* Keep USB middleware debug logging disabled unless the target project
   provides a safe debug output path. */
#define USBD_DEBUG_LEVEL            0U

#define USBD_LPM_ENABLED            0U
#define USBD_SELF_POWERED           1U

/* Bus current draw declared to the host (in units of 2mA).
   50 = 100mA. Adjust if the final target design needs a different value. */
#define USBD_MAX_POWER              50U

/* FS / HS device ID constants — must match DEVICE_FS / DEVICE_HS in usbd_conf.c */
#define DEVICE_FS   0
#define DEVICE_HS   1

/* -------------------------------------------------------------------------- */
/* Memory management macros                                                    */
/* -------------------------------------------------------------------------- */
#define USBD_malloc         malloc
#define USBD_free           free
#define USBD_memset         memset
#define USBD_memcpy         memcpy
#define USBD_Delay          HAL_Delay

/* -------------------------------------------------------------------------- */
/* Debug macros — all disabled (USBD_DEBUG_LEVEL == 0)                        */
/* -------------------------------------------------------------------------- */
#define USBD_UsrLog(...)
#define USBD_ErrLog(...)
#define USBD_DbgLog(...)

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CONF__H__ */
