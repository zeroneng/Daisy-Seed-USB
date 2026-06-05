#ifndef __USB_AUDIO_H
#define __USB_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#define USBD_AUDIO_FREQ             48000U

#define AUDIO_OUT_EP                0x01U
#define AUDIO_IN_EP                 0x81U

#define AUDIO_HS_BINTERVAL          0x01U
#define AUDIO_FS_BINTERVAL          0x01U

#define USB_AUDIO_CONFIG_DESC_SIZ   0xAEU   /* 174 bytes — simplified duplex stereo UAC1 */
#define AUDIO_INTERFACE_DESC_SIZE   0x09U
#define USB_AUDIO_DESC_SIZ          AC_TOTAL_LEN
#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE   0x09U
#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE  0x07U

#define AUDIO_DESCRIPTOR_TYPE               0x21U
#define USB_DEVICE_CLASS_AUDIO              0x01U
#define AUDIO_SUBCLASS_AUDIOCONTROL         0x01U
#define AUDIO_SUBCLASS_AUDIOSTREAMING       0x02U
#define AUDIO_PROTOCOL_UNDEFINED            0x00U
#define AUDIO_STREAMING_GENERAL             0x01U
#define AUDIO_STREAMING_FORMAT_TYPE         0x02U

#define AUDIO_INTERFACE_DESCRIPTOR_TYPE     0x24U
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE      0x25U

#define AUDIO_CONTROL_HEADER                0x01U
#define AUDIO_CONTROL_INPUT_TERMINAL        0x02U
#define AUDIO_CONTROL_OUTPUT_TERMINAL       0x03U
#define AUDIO_CONTROL_FEATURE_UNIT          0x06U

#define AUDIO_INPUT_TERMINAL_DESC_SIZE      0x0CU
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE     0x09U
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE 0x07U

#define AUDIO_CONTROL_MUTE                  0x0001U

#define AUDIO_FORMAT_TYPE_I                 0x01U
#define AUDIO_FORMAT_TYPE_III               0x03U

#define AUDIO_ENDPOINT_GENERAL              0x01U

#define AUDIO_REQ_GET_CUR                   0x81U
#define AUDIO_REQ_SET_CUR                   0x01U

#define AUDIO_OUT_STREAMING_CTRL            0x02U

#define AUDIO_OUT_TC                        0x01U
#define AUDIO_IN_TC                         0x02U

#define AUDIO_OUT_PACKET    ((uint16_t)192U)
#define AUDIO_PACKET_MAX    ((uint16_t)192U)
#define AUDIO_IN_PACKET     AUDIO_OUT_PACKET
#define AUDIO_IN_PACKET_MAX ((uint16_t)192U)
#define AUDIO_CMD_PACKET_SIZE AUDIO_OUT_PACKET

#define AUDIO_DEFAULT_VOLUME    70U
#define AUDIO_TOTAL_BUF_SIZE    ((uint16_t)(AUDIO_PACKET_MAX * 2U))

typedef enum {
    AUDIO_CMD_START = 1,
    AUDIO_CMD_PLAY,
    AUDIO_CMD_STOP,
} AUDIO_CMD_TypeDef;

typedef enum {
    AUDIO_OFFSET_NONE = 0,
    AUDIO_OFFSET_HALF,
    AUDIO_OFFSET_FULL,
    AUDIO_OFFSET_UNKNOWN,
} AUDIO_OffsetTypeDef;

typedef struct {
    uint8_t  cmd;
    uint8_t  data[USB_MAX_EP0_SIZE];
    uint8_t  len;
    uint8_t  unit;
} USBD_AUDIO_ControlTypeDef;

typedef struct {
    uint32_t                  alt_setting;
    uint8_t                   buffer[AUDIO_TOTAL_BUF_SIZE];
    AUDIO_OffsetTypeDef       offset;
    uint8_t                   rd_enable;
    uint16_t                  rd_ptr;
    uint16_t                  wr_ptr;
    USBD_AUDIO_ControlTypeDef control;
} USBD_AUDIO_HandleTypeDef;

typedef struct {
    int8_t   (*Init)      (uint32_t AudioFreq, uint32_t Volume, uint32_t options);
    int8_t   (*DeInit)    (uint32_t options);
    int8_t   (*AudioCmd)  (uint8_t *pbuf, uint32_t size, uint8_t cmd);
    int8_t   (*VolumeCtl) (uint8_t vol);
    int8_t   (*MuteCtl)   (uint8_t cmd);
    uint16_t (*PeriodicTC)(uint8_t *pbuf, uint32_t size, uint8_t cmd);
    int8_t   (*GetState)  (void);
} USBD_AUDIO_ItfTypeDef;

extern USBD_ClassTypeDef USBD_AUDIO;
#define USBD_AUDIO_CLASS &USBD_AUDIO

uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                     USBD_AUDIO_ItfTypeDef *fops);

#ifdef __cplusplus
}
#endif

#endif /* __USB_AUDIO_H */
