#ifndef __USBD_DESC__H__
#define __USBD_DESC__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_def.h"

#define USB_COMP_IDX_CDC_STR   (USBD_IDX_INTERFACE_STR + 1U)
#define USB_COMP_IDX_HID_STR   (USBD_IDX_INTERFACE_STR + 2U)
#define USB_COMP_IDX_AUDIO_STR (USBD_IDX_INTERFACE_STR + 3U)
#define USB_COMP_IDX_MIDI_STR  (USBD_IDX_INTERFACE_STR + 4U)

extern USBD_DescriptorsTypeDef FS_Desc;
extern USBD_DescriptorsTypeDef HS_Desc;

#ifdef __cplusplus
}
#endif

#endif
