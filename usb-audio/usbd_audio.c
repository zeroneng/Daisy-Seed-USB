#include "usbd_audio.h"
#include "usbd_ctlreq.h"

#define HAUDIO  ((USBD_AUDIO_HandleTypeDef *)pdev->pClassData)
#define ITFOPS  ((USBD_AUDIO_ItfTypeDef    *)pdev->pUserData)

static uint8_t  USBD_AUDIO_Init               (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_AUDIO_DeInit             (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_AUDIO_Setup              (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t *USBD_AUDIO_GetCfgDesc         (uint16_t *length);
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t  USBD_AUDIO_DataIn             (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_DataOut            (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_EP0_RxReady        (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_EP0_TxReady        (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_SOF                (USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_IsoINIncomplete    (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_IsoOutIncomplete   (USBD_HandleTypeDef *pdev, uint8_t epnum);
static void     AUDIO_REQ_GetCurrent          (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void     AUDIO_REQ_SetCurrent          (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

USBD_ClassTypeDef USBD_AUDIO = {
    USBD_AUDIO_Init, USBD_AUDIO_DeInit, USBD_AUDIO_Setup,
    USBD_AUDIO_EP0_TxReady, USBD_AUDIO_EP0_RxReady,
    USBD_AUDIO_DataIn, USBD_AUDIO_DataOut, USBD_AUDIO_SOF,
    USBD_AUDIO_IsoINIncomplete, USBD_AUDIO_IsoOutIncomplete,
    USBD_AUDIO_GetCfgDesc, USBD_AUDIO_GetCfgDesc, USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetDeviceQualifierDesc,
};

/* -----------------------------------------------------------------------
 * Configuration descriptor — 109 bytes, UAC1 stereo mic 16-bit 48kHz
 *
 * Changes from working baseline (usbd_audio.c 1771962245333):
 *
 *   [offset 65] bmAttributes:  0x01 → 0x05
 *     0x01 = synchronous isochronous (device slaved to host SOF)
 *     0x05 = asynchronous isochronous (device controls rate)
 *     Async is required for variable packet size to be valid. With sync
 *     the host may enforce exactly 192 bytes and discard 196-byte packets.
 *
 *   [offset 66-67] wMaxPacketSize: 0xC0 0x00 (192) → 0xC4 0x00 (196)
 *     Must fit the largest packet we will ever send (49 frames × 4 bytes).
 *     The host uses this to allocate its receive buffer. Sending 196 bytes
 *     to an endpoint declared as 192 causes silent packet loss.
 * --------------------------------------------------------------------- */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDesc[USB_AUDIO_CONFIG_DESC_SIZ] __ALIGN_END = {
    /* Configuration (9) */
    0x09, USB_DESC_TYPE_CONFIGURATION,
    LOBYTE(USB_AUDIO_CONFIG_DESC_SIZ), HIBYTE(USB_AUDIO_CONFIG_DESC_SIZ),
    0x02, 0x01, 0x00, 0xC0, 50U,

    /* AudioControl Interface (9) */
    0x09, USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x00,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOCONTROL, AUDIO_PROTOCOL_UNDEFINED, 0x00,

    /* AC Header (9) */
    0x09, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_HEADER,
    0x00, 0x01, 0x27, 0x00, 0x01, 0x01,

    /* Input Terminal — Microphone stereo (12) */
    0x0C, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_INPUT_TERMINAL,
    0x01, 0x01, 0x02, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,

    /* Feature Unit — Mute (9) */
    0x09, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_FEATURE_UNIT,
    0x02, 0x01, 0x01, AUDIO_CONTROL_MUTE, 0x00, 0x00,

    /* Output Terminal — USB Streaming (9) */
    0x09, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_OUTPUT_TERMINAL,
    0x03, 0x01, 0x01, 0x00, 0x02, 0x00,

    /* AudioStreaming Interface alt 0 — zero bandwidth (9) */
    0x09, USB_DESC_TYPE_INTERFACE, 0x01, 0x00, 0x00,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00,

    /* AudioStreaming Interface alt 1 — active, 1 endpoint (9) */
    0x09, USB_DESC_TYPE_INTERFACE, 0x01, 0x01, 0x01,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00,

    /* AS General (7) */
    0x07, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_GENERAL,
    0x03, 0x01, 0x01, 0x00,

    /* Type I Format — 2ch 16-bit 48kHz (11) */
    0x0B, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_FORMAT_TYPE,
    AUDIO_FORMAT_TYPE_I, 0x02, 0x02, 16, 0x01,
    0x80, 0xBB, 0x00,   /* 48000 = 0x00BB80 little-endian */

    /* ISO IN Endpoint 0x81 (9) */
    0x09, USB_DESC_TYPE_ENDPOINT, AUDIO_IN_EP,
    0x05,               /* bmAttributes = async isochronous (was 0x01 sync) */
    0xC4, 0x00,         /* wMaxPacketSize = 196 (was 0xC0=192) */
    0x01, 0x00, 0x00,   /* bInterval=1ms, bRefresh=0, bSynchAddress=0 */

    /* AS Endpoint Descriptor (7) */
    0x07, AUDIO_ENDPOINT_DESCRIPTOR_TYPE, AUDIO_ENDPOINT_GENERAL,
    0x00, 0x00, 0x00, 0x00,
};

__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC, USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
};

static uint8_t  audio_streaming_active = 0U;

/* Tracks the byte count of the last submitted packet.
   IsoINIncomplete re-submits the same slot with the same size — no ring
   buffer drain, no double-drain race. */
static uint16_t last_tx_size = AUDIO_OUT_PACKET;

/* -----------------------------------------------------------------------
 * Init
 * --------------------------------------------------------------------- */
static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_AUDIO_HandleTypeDef *haudio =
        (USBD_AUDIO_HandleTypeDef *)USBD_malloc(sizeof(USBD_AUDIO_HandleTypeDef));
    if(haudio == NULL) { pdev->pClassData = NULL; return USBD_FAIL; }

    pdev->pClassData = haudio;

    /* Open endpoint at AUDIO_PACKET_MAX (196) so 49-frame packets are
       accepted. Opening at 192 causes silent rejection of 196-byte packets. */
    USBD_LL_OpenEP(pdev, AUDIO_IN_EP, USBD_EP_TYPE_ISOC, AUDIO_PACKET_MAX);
    pdev->ep_in[AUDIO_IN_EP & 0x7FU].is_used = 1U;

    haudio->alt_setting = 0U;
    haudio->offset      = AUDIO_OFFSET_NONE;
    haudio->wr_ptr      = 0U;
    haudio->rd_ptr      = 0U;
    haudio->rd_enable   = 0U;
    audio_streaming_active = 0U;
    last_tx_size           = AUDIO_OUT_PACKET;

    if(ITFOPS->Init(USBD_AUDIO_FREQ, AUDIO_DEFAULT_VOLUME, 0U) != 0)
        return USBD_FAIL;

    memset(haudio->buffer, 0, sizeof(haudio->buffer));
    USBD_LL_Transmit(pdev, AUDIO_IN_EP, haudio->buffer, AUDIO_OUT_PACKET);
    return USBD_OK;
}

static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_LL_CloseEP(pdev, AUDIO_IN_EP);
    pdev->ep_in[AUDIO_IN_EP & 0x7FU].is_used = 0U;
    audio_streaming_active = 0U;

    if(pdev->pClassData != NULL) {
        ITFOPS->DeInit(0U);
        USBD_free(pdev->pClassData);
        pdev->pClassData = NULL;
    }
    return USBD_OK;
}

/* -----------------------------------------------------------------------
 * Setup — identical to working baseline
 * --------------------------------------------------------------------- */
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    uint16_t len;
    uint16_t status_info = 0U;
    USBD_StatusTypeDef ret = USBD_OK;

    if(haudio == NULL) return USBD_FAIL;

    switch(req->bmRequest & USB_REQ_TYPE_MASK)
    {
        case USB_REQ_TYPE_CLASS:
            switch(req->bRequest) {
                case AUDIO_REQ_GET_CUR: AUDIO_REQ_GetCurrent(pdev, req); break;
                case AUDIO_REQ_SET_CUR: AUDIO_REQ_SetCurrent(pdev, req); break;
                default: USBD_CtlError(pdev, req); ret = USBD_FAIL; break;
            }
            break;

        case USB_REQ_TYPE_STANDARD:
            switch(req->bRequest) {
                case USB_REQ_GET_STATUS:
                    if(pdev->dev_state == USBD_STATE_CONFIGURED)
                        USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
                    else { USBD_CtlError(pdev, req); ret = USBD_FAIL; }
                    break;

                case USB_REQ_GET_DESCRIPTOR:
                    if((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE) {
                        len = MIN(USB_AUDIO_DESC_SIZ, req->wLength);
                        USBD_CtlSendData(pdev, USBD_AUDIO_CfgDesc + 18U, len);
                    }
                    break;

                case USB_REQ_GET_INTERFACE:
                    if(pdev->dev_state == USBD_STATE_CONFIGURED)
                        USBD_CtlSendData(pdev, (uint8_t *)&haudio->alt_setting, 1U);
                    else { USBD_CtlError(pdev, req); ret = USBD_FAIL; }
                    break;

                case USB_REQ_SET_INTERFACE:
                    if(pdev->dev_state == USBD_STATE_CONFIGURED) {
                        haudio->alt_setting = (uint8_t)(req->wValue);
                        if(haudio->alt_setting == 1U) {
                            audio_streaming_active = 1U;
                            haudio->wr_ptr = 0U;
                            last_tx_size   = AUDIO_OUT_PACKET;
                            memset(haudio->buffer, 0, sizeof(haudio->buffer));
                            USBD_LL_Transmit(pdev, AUDIO_IN_EP,
                                             haudio->buffer, AUDIO_OUT_PACKET);
                        } else {
                            audio_streaming_active = 0U;
                        }
                    } else { USBD_CtlError(pdev, req); ret = USBD_FAIL; }
                    break;

                default: USBD_CtlError(pdev, req); ret = USBD_FAIL; break;
            }
            break;

        default: USBD_CtlError(pdev, req); ret = USBD_FAIL; break;
    }
    return ret;
}

/* -----------------------------------------------------------------------
 * DataIn — called after host consumes each isochronous packet
 *
 * Changes from working baseline:
 *
 *   1. XOR toggle replaced with explicit 0/AUDIO_PACKET_MAX alternation.
 *      Old: haudio->wr_ptr ^= AUDIO_OUT_PACKET  (only correct when both
 *           slots are exactly 192 bytes apart; breaks with 196-byte slots)
 *      New: explicit ternary toggle between 0 and AUDIO_PACKET_MAX
 *
 *   2. PeriodicTC return value captured and passed to USBD_LL_Transmit.
 *      Old: return value discarded, AUDIO_OUT_PACKET (192) always sent
 *      New: actual byte count (188/192/196) reaches the wire
 *
 *   3. last_tx_size saved for IsoINIncomplete re-submission.
 * --------------------------------------------------------------------- */
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)epnum;
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL || !audio_streaming_active) return USBD_OK;

    haudio->wr_ptr = (haudio->wr_ptr == 0U) ? AUDIO_PACKET_MAX : 0U;

    last_tx_size = ITFOPS->PeriodicTC(
        &haudio->buffer[haudio->wr_ptr], AUDIO_PACKET_MAX, AUDIO_IN_TC);

    /* Flush D-cache for this buffer slot before DMA reads it.
       Without this, USB DMA reads stale data from physical SRAM
       while the CPU cache holds the fresh data from PeriodicTC. */
    SCB_CleanDCache_by_Addr(
        (uint32_t *)&haudio->buffer[haudio->wr_ptr], last_tx_size);

    USBD_LL_Transmit(pdev, AUDIO_IN_EP,
                     &haudio->buffer[haudio->wr_ptr], last_tx_size);
    return USBD_OK;
}
/* -----------------------------------------------------------------------
 * IsoINIncomplete — packet was lost on the bus (USB frame overrun)
 *
 * Re-submit the current slot verbatim. Do NOT call PeriodicTC — that
 * would drain an extra packet from the ring buffer causing underrun.
 * Use last_tx_size so the re-submitted packet matches the original size.
 * --------------------------------------------------------------------- */
static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)epnum;
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL || !audio_streaming_active) return USBD_OK;

    USBD_LL_FlushEP(pdev, AUDIO_IN_EP);
    USBD_LL_Transmit(pdev, AUDIO_IN_EP,
                     &haudio->buffer[haudio->wr_ptr], last_tx_size);
    return USBD_OK;
}

/* -----------------------------------------------------------------------
 * Stubs — identical to working baseline
 * --------------------------------------------------------------------- */
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev)
    { (void)pdev; return USBD_OK; }
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
    { (void)pdev; (void)epnum; return USBD_OK; }
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
    { (void)pdev; (void)epnum; return USBD_OK; }
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev)
    { (void)pdev; return USBD_OK; }

static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length)
    { *length = sizeof(USBD_AUDIO_CfgDesc); return USBD_AUDIO_CfgDesc; }
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length)
    { *length = sizeof(USBD_AUDIO_DeviceQualifierDesc); return USBD_AUDIO_DeviceQualifierDesc; }

static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL) return USBD_FAIL;
    if(haudio->control.cmd == AUDIO_REQ_SET_CUR) {
        ITFOPS->MuteCtl(haudio->control.data[0]);
        haudio->control.cmd = 0U;
        haudio->control.len = 0U;
    }
    return USBD_OK;
}

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL) return;
    memset(haudio->control.data, 0, USB_MAX_EP0_SIZE);
    USBD_CtlSendData(pdev, haudio->control.data, MIN(req->wLength, USB_MAX_EP0_SIZE));
}

static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL) return;
    if(req->wLength != 0U) {
        haudio->control.cmd  = AUDIO_REQ_SET_CUR;
        haudio->control.len  = (uint8_t)MIN(req->wLength, USB_MAX_EP0_SIZE);
        haudio->control.unit = HIBYTE(req->wIndex);
        USBD_CtlPrepareRx(pdev, haudio->control.data, haudio->control.len);
    }
}

uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                     USBD_AUDIO_ItfTypeDef *fops)
{
    if(fops == NULL) return USBD_FAIL;
    pdev->pUserData = fops;
    return USBD_OK;
}
