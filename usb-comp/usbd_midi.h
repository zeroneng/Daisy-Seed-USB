#ifndef __USB_MIDI_H
#define __USB_MIDI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#define MIDI_OUT_EP                         0x03U
#define MIDI_IN_EP                          0x85U
#define MIDI_DATA_FS_MAX_PACKET_SIZE        64U
#define MIDI_DATA_HS_MAX_PACKET_SIZE        512U
#define MIDI_DATA_FS_IN_PACKET_SIZE         MIDI_DATA_FS_MAX_PACKET_SIZE
#define MIDI_DATA_FS_OUT_PACKET_SIZE        MIDI_DATA_FS_MAX_PACKET_SIZE
#define MIDI_DATA_HS_IN_PACKET_SIZE         MIDI_DATA_HS_MAX_PACKET_SIZE
#define MIDI_DATA_HS_OUT_PACKET_SIZE        MIDI_DATA_HS_MAX_PACKET_SIZE
#define MIDI_USB_EVENT_PACKET_SIZE          4U

#ifndef USB_DEVICE_CLASS_AUDIO
#define USB_DEVICE_CLASS_AUDIO              0x01U
#endif

#ifndef AUDIO_SUBCLASS_AUDIOCONTROL
#define AUDIO_SUBCLASS_AUDIOCONTROL         0x01U
#endif

#define AUDIO_SUBCLASS_MIDISTREAMING        0x03U

#ifndef AUDIO_PROTOCOL_UNDEFINED
#define AUDIO_PROTOCOL_UNDEFINED            0x00U
#endif

#ifndef AUDIO_INTERFACE_DESCRIPTOR_TYPE
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE      0x24U
#endif

#ifndef AUDIO_ENDPOINT_DESCRIPTOR_TYPE
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE       0x25U
#endif

#define MIDI_STREAMING_HEADER               0x01U
#define MIDI_IN_JACK                        0x02U
#define MIDI_OUT_JACK                       0x03U
#define MIDI_ENDPOINT_GENERAL               0x01U

typedef struct
{
    uint8_t *RxBuffer;
    uint8_t *TxBuffer;
    uint32_t RxLength;
    uint32_t TxLength;
    __IO uint32_t TxState;
}
USBD_MIDI_HandleTypeDef;

typedef struct
{
    int8_t (*Init)(void);
    int8_t (*DeInit)(void);
    int8_t (*Receive)(uint8_t *Buf, uint32_t *Len);
    int8_t (*TransmitCplt)(uint8_t *Buf, uint32_t *Len, uint8_t epnum);
}
USBD_MIDI_ItfTypeDef;

extern USBD_ClassTypeDef USBD_MIDI;
#define USBD_MIDI_CLASS &USBD_MIDI

uint8_t USBD_MIDI_RegisterInterface(USBD_HandleTypeDef *pdev,
                                    USBD_MIDI_ItfTypeDef *fops);
uint8_t USBD_MIDI_SetTxBuffer(USBD_HandleTypeDef *pdev,
                              uint8_t *pbuff,
                              uint16_t length,
                              uint8_t class_id);
uint8_t USBD_MIDI_SetRxBuffer(USBD_HandleTypeDef *pdev,
                              uint8_t *pbuff,
                              uint8_t class_id);
uint8_t USBD_MIDI_TransmitPacket(USBD_HandleTypeDef *pdev, uint8_t class_id);
uint8_t USBD_MIDI_ReceivePacket(USBD_HandleTypeDef *pdev, uint8_t class_id);

#ifdef __cplusplus
}
#endif

#endif /* __USB_MIDI_H */
