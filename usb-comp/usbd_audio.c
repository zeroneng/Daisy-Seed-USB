#include "usbd_audio.h"
#include "usbd_ctlreq.h"
#include <string.h>

#ifdef USE_USBD_COMPOSITE
#define HAUDIO  ((USBD_AUDIO_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId])
#define ITFOPS  ((USBD_AUDIO_ItfTypeDef    *)pdev->pUserData[pdev->classId])
#define AUDIO_AC_IF(pdev)  ((pdev)->tclasslist[(pdev)->classId].Ifs[0])
#define AUDIO_OUT_IF(pdev) ((pdev)->tclasslist[(pdev)->classId].Ifs[1])
#define AUDIO_IN_IF(pdev)  ((pdev)->tclasslist[(pdev)->classId].Ifs[2])
#else
#define HAUDIO  ((USBD_AUDIO_HandleTypeDef *)pdev->pClassData)
#define ITFOPS  ((USBD_AUDIO_ItfTypeDef    *)pdev->pUserData)
#define AUDIO_AC_IF(pdev)  0U
#define AUDIO_OUT_IF(pdev) 1U
#define AUDIO_IN_IF(pdev)  2U
#endif
#define AC_TOTAL_LEN 0x34U

static uint8_t  USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t  USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);
static uint8_t  USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static void     AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void     AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

USBD_ClassTypeDef USBD_AUDIO = {
    USBD_AUDIO_Init, USBD_AUDIO_DeInit, USBD_AUDIO_Setup,
    USBD_AUDIO_EP0_TxReady, USBD_AUDIO_EP0_RxReady,
    USBD_AUDIO_DataIn, USBD_AUDIO_DataOut, USBD_AUDIO_SOF,
    USBD_AUDIO_IsoINIncomplete, USBD_AUDIO_IsoOutIncomplete,
    USBD_AUDIO_GetCfgDesc, USBD_AUDIO_GetCfgDesc, USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetDeviceQualifierDesc,
};

__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDesc[USB_AUDIO_CONFIG_DESC_SIZ] __ALIGN_END = {
    0x09, USB_DESC_TYPE_CONFIGURATION,
    LOBYTE(USB_AUDIO_CONFIG_DESC_SIZ), HIBYTE(USB_AUDIO_CONFIG_DESC_SIZ),
    0x03, 0x01, 0x00, 0x80, 0xFA,

    /* AC interface 0 */
    0x09, USB_DESC_TYPE_INTERFACE,
    0x00, 0x00, 0x00,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOCONTROL, AUDIO_PROTOCOL_UNDEFINED, 0x00,

    /* AC header: two streaming interfaces */
    0x0A, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_HEADER,
    0x00, 0x01,
    LOBYTE(AC_TOTAL_LEN), HIBYTE(AC_TOTAL_LEN),
    0x02,
    0x01,
    0x02,

    /* USB-OUT -> IT_1 -> OT_2 -> playback sink */
    0x0C, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_INPUT_TERMINAL,
    0x01,
    0x01, 0x01,
    0x00,
    0x02,
    0x03, 0x00,
    0x00,
    0x00,

    0x09, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_OUTPUT_TERMINAL,
    0x02,
    0x01, 0x03,
    0x00,
    0x01,
    0x00,

    /* capture source -> IT_3 -> OT_4 -> USB-IN */
    0x0C, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_INPUT_TERMINAL,
    0x03,
    0x01, 0x02,
    0x00,
    0x02,
    0x03, 0x00,
    0x00,
    0x00,

    0x09, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_OUTPUT_TERMINAL,
    0x04,
    0x01, 0x01,
    0x00,
    0x03,
    0x00,

    /* AS OUT interface 1 alt 0 */
    0x09, USB_DESC_TYPE_INTERFACE,
    0x01, 0x00, 0x00,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00,

    /* AS OUT interface 1 alt 1 */
    0x09, USB_DESC_TYPE_INTERFACE,
    0x01, 0x01, 0x01,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00,

    /* AS OUT general links to IT_1 */
    0x07, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_GENERAL,
    0x01,
    0x01,
    0x01, 0x00,

    /* AS OUT format */
    0x0B, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_FORMAT_TYPE,
    AUDIO_FORMAT_TYPE_I,
    0x02,
    0x02,
    16,
    0x01,
    0x80, 0xBB, 0x00,

    /* OUT endpoint */
    0x09, USB_DESC_TYPE_ENDPOINT,
    AUDIO_OUT_EP,
    0x09,
    LOBYTE(AUDIO_OUT_PACKET), HIBYTE(AUDIO_OUT_PACKET),
    0x01,
    0x00,
    0x00,

    /* OUT class-specific endpoint */
    0x07, AUDIO_ENDPOINT_DESCRIPTOR_TYPE, AUDIO_ENDPOINT_GENERAL,
    0x01,
    0x01,
    0x01, 0x00,

    /* AS IN interface 2 alt 0 */
    0x09, USB_DESC_TYPE_INTERFACE,
    0x02, 0x00, 0x00,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00,

    /* AS IN interface 2 alt 1 */
    0x09, USB_DESC_TYPE_INTERFACE,
    0x02, 0x01, 0x01,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00,

    /* AS IN general links to OT_4 */
    0x07, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_GENERAL,
    0x04,
    0x01,
    0x01, 0x00,

    /* AS IN format */
    0x0B, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_FORMAT_TYPE,
    AUDIO_FORMAT_TYPE_I,
    0x02,
    0x02,
    16,
    0x01,
    0x80, 0xBB, 0x00,

    /* IN endpoint */
    0x09, USB_DESC_TYPE_ENDPOINT,
    AUDIO_IN_EP,
    0x05,
    LOBYTE(AUDIO_IN_PACKET_MAX), HIBYTE(AUDIO_IN_PACKET_MAX),
    0x01,
    0x00,
    0x00,

    /* IN class-specific endpoint */
    0x07, AUDIO_ENDPOINT_DESCRIPTOR_TYPE, AUDIO_ENDPOINT_GENERAL,
    0x01,
    0x00,
    0x00, 0x00,
};

__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC, USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
};

static uint8_t  audio_in_streaming_active  = 0U;
static uint8_t  audio_out_streaming_active = 0U;
static uint8_t  audio_in_prime_pending     = 0U;
static uint16_t last_tx_size               = AUDIO_OUT_PACKET;
static uint8_t  out_rx_buf[AUDIO_CMD_PACKET_SIZE];

static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_AUDIO_HandleTypeDef *haudio = (USBD_AUDIO_HandleTypeDef *)USBD_malloc(sizeof(USBD_AUDIO_HandleTypeDef));
    if(haudio == NULL)
    {
        pdev->pClassData = NULL;
        return USBD_FAIL;
    }

#ifdef USE_USBD_COMPOSITE
    pdev->pClassDataCmsit[pdev->classId] = haudio;
    pdev->pClassData = pdev->pClassDataCmsit[pdev->classId];
#else
    pdev->pClassData = haudio;
#endif
    memset(haudio, 0, sizeof(*haudio));
    memset(out_rx_buf, 0, sizeof(out_rx_buf));

    USBD_LL_OpenEP(pdev, AUDIO_IN_EP, USBD_EP_TYPE_ISOC, AUDIO_IN_PACKET_MAX);
    USBD_LL_OpenEP(pdev, AUDIO_OUT_EP, USBD_EP_TYPE_ISOC, AUDIO_OUT_PACKET);
    pdev->ep_in[AUDIO_IN_EP & 0x7FU].is_used   = 1U;
    pdev->ep_out[AUDIO_OUT_EP & 0x7FU].is_used = 1U;

    if(ITFOPS->Init(USBD_AUDIO_FREQ, AUDIO_DEFAULT_VOLUME, 0U) != 0)
        return USBD_FAIL;

    memset(haudio->buffer, 0, sizeof(haudio->buffer));
    last_tx_size = AUDIO_OUT_PACKET;
    return USBD_OK;
}

static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_LL_CloseEP(pdev, AUDIO_IN_EP);
    USBD_LL_CloseEP(pdev, AUDIO_OUT_EP);
    pdev->ep_in[AUDIO_IN_EP & 0x7FU].is_used   = 0U;
    pdev->ep_out[AUDIO_OUT_EP & 0x7FU].is_used = 0U;
    audio_in_streaming_active  = 0U;
    audio_out_streaming_active = 0U;
    audio_in_prime_pending     = 0U;

    if(HAUDIO != NULL)
    {
        ITFOPS->DeInit(0U);
        USBD_free(HAUDIO);
#ifdef USE_USBD_COMPOSITE
        pdev->pClassDataCmsit[pdev->classId] = NULL;
#endif
        pdev->pClassData = NULL;
    }
    return USBD_OK;
}

static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    uint16_t len;
    uint16_t status_info = 0U;
    USBD_StatusTypeDef ret = USBD_OK;

    if(haudio == NULL)
    {
        return USBD_FAIL;
    }

    switch(req->bmRequest & USB_REQ_TYPE_MASK)
    {
        case USB_REQ_TYPE_CLASS:
            switch(req->bRequest)
            {
                case AUDIO_REQ_GET_CUR: AUDIO_REQ_GetCurrent(pdev, req); break;
                case AUDIO_REQ_SET_CUR: AUDIO_REQ_SetCurrent(pdev, req); break;
                default: USBD_CtlError(pdev, req); ret = USBD_FAIL; break;
            }
            break;

        case USB_REQ_TYPE_STANDARD:
            switch(req->bRequest)
            {
                case USB_REQ_GET_STATUS:
                    if(pdev->dev_state == USBD_STATE_CONFIGURED)
                        USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
                    else
                    {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_GET_DESCRIPTOR:
                    if((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
                    {
                        len = MIN((uint16_t)AC_TOTAL_LEN, req->wLength);
                        USBD_CtlSendData(pdev, USBD_AUDIO_CfgDesc + 18U, len);
                    }
                    else
                    {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_GET_INTERFACE:
                    if(pdev->dev_state == USBD_STATE_CONFIGURED)
                    {
                        uint8_t ifnum = LOBYTE(req->wIndex);
                        uint8_t alt = 0U;
                        if(ifnum == AUDIO_OUT_IF(pdev)) alt = audio_out_streaming_active ? 1U : 0U;
                        else if(ifnum == AUDIO_IN_IF(pdev)) alt = audio_in_streaming_active ? 1U : 0U;
                        USBD_CtlSendData(pdev, &alt, 1U);
                    }
                    else
                    {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_SET_INTERFACE:
                    if(pdev->dev_state == USBD_STATE_CONFIGURED)
                    {
                        uint8_t ifnum = LOBYTE(req->wIndex);
                        uint8_t alt = (uint8_t)(req->wValue);
                        if(ifnum == AUDIO_OUT_IF(pdev))
                        {
                            audio_out_streaming_active = (alt == 1U) ? 1U : 0U;
                            if(audio_out_streaming_active)
                            {
                                memset(out_rx_buf, 0, sizeof(out_rx_buf));
                                USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, out_rx_buf, AUDIO_OUT_PACKET);
                            }
                        }
                        else if(ifnum == AUDIO_IN_IF(pdev))
                        {
                            audio_in_streaming_active = (alt == 1U) ? 1U : 0U;
                            if(audio_in_streaming_active)
                            {
                                haudio->wr_ptr = 0U;
                                last_tx_size   = AUDIO_OUT_PACKET;
                                memset(haudio->buffer, 0, sizeof(haudio->buffer));
                                audio_in_prime_pending = 1U;
                            }
                            else
                            {
                                audio_in_prime_pending = 0U;
                            }
                        }
                    }
                    else
                    {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                default: USBD_CtlError(pdev, req); ret = USBD_FAIL; break;
            }
            break;

        default: USBD_CtlError(pdev, req); ret = USBD_FAIL; break;
    }
    return ret;
}

static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)epnum;
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL || !audio_in_streaming_active)
        return USBD_OK;

    haudio->wr_ptr = (haudio->wr_ptr == 0U) ? AUDIO_PACKET_MAX : 0U;
    last_tx_size = ITFOPS->PeriodicTC(&haudio->buffer[haudio->wr_ptr], AUDIO_PACKET_MAX, AUDIO_IN_TC);
    SCB_CleanDCache_by_Addr((uint32_t *)&haudio->buffer[haudio->wr_ptr], last_tx_size);
    USBD_LL_Transmit(pdev, AUDIO_IN_EP, &haudio->buffer[haudio->wr_ptr], last_tx_size);
    return USBD_OK;
}

static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)epnum;
    if(!audio_out_streaming_active)
        return USBD_OK;
    uint32_t rx_size = USBD_LL_GetRxDataSize(pdev, AUDIO_OUT_EP);
    ITFOPS->AudioCmd(out_rx_buf, rx_size, AUDIO_OUT_TC);
    USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, out_rx_buf, AUDIO_OUT_PACKET);
    return USBD_OK;
}

static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)epnum;
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL || !audio_in_streaming_active)
        return USBD_OK;
    USBD_LL_FlushEP(pdev, AUDIO_IN_EP);
    USBD_LL_Transmit(pdev, AUDIO_IN_EP, &haudio->buffer[haudio->wr_ptr], last_tx_size);
    return USBD_OK;
}

static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)epnum;
    if(!audio_out_streaming_active)
        return USBD_OK;
    USBD_LL_FlushEP(pdev, AUDIO_OUT_EP);
    USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, out_rx_buf, AUDIO_OUT_PACKET);
    return USBD_OK;
}

static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(audio_in_streaming_active && audio_in_prime_pending && haudio != NULL)
    {
        audio_in_prime_pending = 0U;
        last_tx_size = ITFOPS->PeriodicTC(&haudio->buffer[haudio->wr_ptr], AUDIO_PACKET_MAX, AUDIO_IN_TC);
        SCB_CleanDCache_by_Addr((uint32_t *)&haudio->buffer[haudio->wr_ptr], last_tx_size);
        USBD_LL_Transmit(pdev, AUDIO_IN_EP, &haudio->buffer[haudio->wr_ptr], last_tx_size);
    }
    return USBD_OK;
}

static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
    (void)pdev;
    return USBD_OK;
}

static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length)
{
    *length = sizeof(USBD_AUDIO_CfgDesc);
    return USBD_AUDIO_CfgDesc;
}

static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length)
{
    *length = sizeof(USBD_AUDIO_DeviceQualifierDesc);
    return USBD_AUDIO_DeviceQualifierDesc;
}

static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL)
        return USBD_FAIL;
    if(haudio->control.cmd == AUDIO_REQ_SET_CUR)
    {
        ITFOPS->MuteCtl(haudio->control.data[0]);
        haudio->control.cmd = 0U;
        haudio->control.len = 0U;
    }
    return USBD_OK;
}

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL)
        return;
    memset(haudio->control.data, 0, USB_MAX_EP0_SIZE);

    if((HIBYTE(req->wValue) == 0x01U) && (req->wLength == 3U))
    {
        haudio->control.data[0] = 0x80U;
        haudio->control.data[1] = 0xBBU;
        haudio->control.data[2] = 0x00U;
        USBD_CtlSendData(pdev, haudio->control.data, MIN(req->wLength, 3U));
        return;
    }

    USBD_CtlSendData(pdev, haudio->control.data, MIN(req->wLength, USB_MAX_EP0_SIZE));
}

static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_AUDIO_HandleTypeDef *haudio = HAUDIO;
    if(haudio == NULL)
        return;
    if(req->wLength != 0U)
    {
        haudio->control.cmd  = AUDIO_REQ_SET_CUR;
        haudio->control.len  = (uint8_t)MIN(req->wLength, USB_MAX_EP0_SIZE);
        haudio->control.unit = HIBYTE(req->wIndex);
        USBD_CtlPrepareRx(pdev, haudio->control.data, haudio->control.len);
    }
}

uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_AUDIO_ItfTypeDef *fops)
{
    if(fops == NULL)
        return USBD_FAIL;
#ifdef USE_USBD_COMPOSITE
    pdev->pUserData[pdev->classId] = fops;
#else
    pdev->pUserData = fops;
#endif
    return USBD_OK;
}
