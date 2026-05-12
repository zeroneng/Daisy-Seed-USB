/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @brief   USB Device configuration for Daisy Seed UAC1 Microphone
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

/* UAC1 needs at minimum 2 interfaces: AudioControl + AudioStreaming.
   Keep at 2. If you later add a MIDI or CDC interface alongside audio,
   increase this. Setting it to 1 (the libDaisy CDC default) will cause
   SET_INTERFACE requests to be NAK'd and enumeration will fail. */
#define USBD_MAX_NUM_INTERFACES     2U

#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       512U
#define USBD_SUPPORT_USER_STRING    1U

/* Set to 0 — we have no USB serial port when running as audio device,
   so printf goes nowhere and USBD_DEBUG_LEVEL > 0 will loop on printf
   with undefined behaviour. */
#define USBD_DEBUG_LEVEL            0U

#define USBD_LPM_ENABLED            0U
#define USBD_SELF_POWERED           1U

/* Bus current draw declared to the host (in units of 2mA).
   50 = 100mA. Daisy Seed draws ~70mA under load.
   Missing this define causes a compile error in usbd_audio.c. */
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
