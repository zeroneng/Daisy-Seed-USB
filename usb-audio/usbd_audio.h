#ifndef __USB_AUDIO_H
#define __USB_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#define USBD_AUDIO_FREQ             48000U
#define USBD_MAX_NUM_INTERFACES     2U

#define AUDIO_OUT_EP                0x01U
#define AUDIO_IN_EP                 0x81U

#define AUDIO_HS_BINTERVAL          0x01U
#define AUDIO_FS_BINTERVAL          0x01U

#define USB_AUDIO_CONFIG_DESC_SIZ   0x6DU   /* 109 bytes — input-only stereo capture */
#define AUDIO_INTERFACE_DESC_SIZE   0x09U
#define USB_AUDIO_DESC_SIZ          0x09U
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

/* -----------------------------------------------------------------------
 * Packet size constants
 *
 * Stereo 16-bit 48kHz capture:
 * 48 frames × 2ch × 2B = 192 bytes nominal
 * 49 frames × 2ch × 2B = 196 bytes maximum
 * --------------------------------------------------------------------- */
#define AUDIO_OUT_PACKET    (uint16_t)(((USBD_AUDIO_FREQ * 2U * 2U) / 1000U))       /* 192 */
#define AUDIO_PACKET_MAX    (uint16_t)((((USBD_AUDIO_FREQ / 1000U) + 1U) * 2U * 2U)) /* 196 */
#define AUDIO_IN_PACKET     AUDIO_OUT_PACKET
#define AUDIO_IN_PACKET_MAX AUDIO_PACKET_MAX

#define AUDIO_DEFAULT_VOLUME    70U

/* Double-buffer: two slots each sized for the largest possible packet.
   Previous value (AUDIO_OUT_PACKET * 80 = 15360B) was a legacy playback
   artefact — completely unnecessary for an input-only device.
   New value: 2 × 196 = 392 bytes. */
#define AUDIO_TOTAL_BUF_SIZE    ((uint16_t)(AUDIO_PACKET_MAX * 2U))   /* 392 bytes */

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
    uint8_t                   buffer[AUDIO_TOTAL_BUF_SIZE]; /* double-buffer: slot0=[0..195], slot1=[196..391] */
    AUDIO_OffsetTypeDef       offset;
    uint8_t                   rd_enable;
    uint16_t                  rd_ptr;
    uint16_t                  wr_ptr;                       /* 0 or AUDIO_PACKET_MAX */
    USBD_AUDIO_ControlTypeDef control;
} USBD_AUDIO_HandleTypeDef;

/* -----------------------------------------------------------------------
 * PeriodicTC returns uint16_t — the number of bytes written to pbuf.
 * DataIn passes this directly to USBD_LL_Transmit, so variable packet
 * size (188/192/196 bytes) reaches the wire without any other changes.
 * Previously returned int8_t USBD_OK which forced a hardcoded 192.
 * --------------------------------------------------------------------- */
typedef struct {
    int8_t   (*Init)      (uint32_t AudioFreq, uint32_t Volume, uint32_t options);
    int8_t   (*DeInit)    (uint32_t options);
    int8_t   (*AudioCmd)  (uint8_t *pbuf, uint32_t size, uint8_t cmd);
    int8_t   (*VolumeCtl) (uint8_t vol);
    int8_t   (*MuteCtl)   (uint8_t cmd);
    uint16_t (*PeriodicTC)(uint8_t *pbuf, uint32_t size, uint8_t cmd); /* returns bytes to transmit */
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
