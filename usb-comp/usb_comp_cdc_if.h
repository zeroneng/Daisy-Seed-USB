#ifndef __USB_COMP_CDC_IF_H__
#define __USB_COMP_CDC_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_cdc.h"

typedef void (*CDC_ReceiveCallback)(uint8_t* buf, uint32_t* size);

extern USBD_CDC_ItfTypeDef USB_COMP_CDC_Interface_fops_HS;

void USB_COMP_CDC_SetClassId(uint8_t class_id);
uint8_t USB_COMP_CDC_IsConnected(void);
void CDC_Set_Rx_Callback_HS(CDC_ReceiveCallback cb);
uint8_t CDC_Transmit_HS(uint8_t* Buf, uint16_t Len);

#ifdef __cplusplus
}
#endif

#endif
