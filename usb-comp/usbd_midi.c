#include "usbd_midi.h"
#include "usbd_ctlreq.h"
#include <string.h>

#ifdef USE_USBD_COMPOSITE
#define HMIDI ((USBD_MIDI_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId])
#define ITFOPS ((USBD_MIDI_ItfTypeDef *)pdev->pUserData[pdev->classId])
#else
#define HMIDI ((USBD_MIDI_HandleTypeDef *)pdev->pClassData)
#define ITFOPS ((USBD_MIDI_ItfTypeDef *)pdev->pUserData)
#endif

static uint8_t USBD_MIDI_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_MIDI_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_MIDI_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_MIDI_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_MIDI_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);

USBD_ClassTypeDef USBD_MIDI = {
    USBD_MIDI_Init,
    USBD_MIDI_DeInit,
    USBD_MIDI_Setup,
    NULL,
    NULL,
    USBD_MIDI_DataIn,
    USBD_MIDI_DataOut,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#if (USBD_SUPPORT_USER_STRING_DESC == 1U)
    NULL,
#endif
};

static uint8_t MIDIInEpAdd  = MIDI_IN_EP;
static uint8_t MIDIOutEpAdd = MIDI_OUT_EP;

static uint8_t USBD_MIDI_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)USBD_malloc(sizeof(USBD_MIDI_HandleTypeDef));
    if(hmidi == NULL)
    {
        return USBD_FAIL;
    }
    memset(hmidi, 0, sizeof(*hmidi));

#ifdef USE_USBD_COMPOSITE
    pdev->pClassDataCmsit[pdev->classId] = hmidi;
    pdev->pClassData = hmidi;
    MIDIInEpAdd  = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
    MIDIOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
#else
    pdev->pClassData = hmidi;
#endif

    if(pdev->dev_speed == USBD_SPEED_HIGH)
    {
        (void)USBD_LL_OpenEP(pdev, MIDIInEpAdd, USBD_EP_TYPE_BULK, MIDI_DATA_HS_IN_PACKET_SIZE);
        (void)USBD_LL_OpenEP(pdev, MIDIOutEpAdd, USBD_EP_TYPE_BULK, MIDI_DATA_HS_OUT_PACKET_SIZE);
    }
    else
    {
        (void)USBD_LL_OpenEP(pdev, MIDIInEpAdd, USBD_EP_TYPE_BULK, MIDI_DATA_FS_IN_PACKET_SIZE);
        (void)USBD_LL_OpenEP(pdev, MIDIOutEpAdd, USBD_EP_TYPE_BULK, MIDI_DATA_FS_OUT_PACKET_SIZE);
    }

    pdev->ep_in[MIDIInEpAdd & 0xFU].is_used = 1U;
    pdev->ep_out[MIDIOutEpAdd & 0xFU].is_used = 1U;

    if(ITFOPS != NULL)
    {
        ITFOPS->Init();
    }

    if(hmidi->RxBuffer != NULL)
    {
        (void)USBD_LL_PrepareReceive(pdev, MIDIOutEpAdd, hmidi->RxBuffer, MIDI_DATA_FS_OUT_PACKET_SIZE);
    }

    return USBD_OK;
}

static uint8_t USBD_MIDI_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
#ifdef USE_USBD_COMPOSITE
    MIDIInEpAdd  = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
    MIDIOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
#endif
    (void)USBD_LL_CloseEP(pdev, MIDIInEpAdd);
    (void)USBD_LL_CloseEP(pdev, MIDIOutEpAdd);
    pdev->ep_in[MIDIInEpAdd & 0xFU].is_used = 0U;
    pdev->ep_out[MIDIOutEpAdd & 0xFU].is_used = 0U;

    if(HMIDI != NULL)
    {
        if(ITFOPS != NULL)
        {
            ITFOPS->DeInit();
        }
        (void)USBD_free(HMIDI);
#ifdef USE_USBD_COMPOSITE
        pdev->pClassDataCmsit[pdev->classId] = NULL;
#endif
        pdev->pClassData = NULL;
    }

    return USBD_OK;
}

static uint8_t USBD_MIDI_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    uint16_t status_info = 0U;

    switch(req->bmRequest & USB_REQ_TYPE_MASK)
    {
        case USB_REQ_TYPE_STANDARD:
            if(req->bRequest == USB_REQ_GET_STATUS && pdev->dev_state == USBD_STATE_CONFIGURED)
            {
                USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
                return USBD_OK;
            }
            break;
        default:
            break;
    }

    USBD_CtlError(pdev, req);
    return USBD_FAIL;
}

static uint8_t USBD_MIDI_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    USBD_MIDI_HandleTypeDef *hmidi = HMIDI;
    if(hmidi == NULL)
    {
        return USBD_FAIL;
    }

    hmidi->TxState = 0U;
    if(ITFOPS != NULL && ITFOPS->TransmitCplt != NULL)
    {
        ITFOPS->TransmitCplt(hmidi->TxBuffer, &hmidi->TxLength, epnum);
    }

    return USBD_OK;
}

static uint8_t USBD_MIDI_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)epnum;
    USBD_MIDI_HandleTypeDef *hmidi = HMIDI;
    if(hmidi == NULL)
    {
        return USBD_FAIL;
    }

#ifdef USE_USBD_COMPOSITE
    MIDIOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
#endif
    hmidi->RxLength = USBD_LL_GetRxDataSize(pdev, MIDIOutEpAdd);

    if(ITFOPS != NULL && ITFOPS->Receive != NULL)
    {
        ITFOPS->Receive(hmidi->RxBuffer, &hmidi->RxLength);
    }

    return USBD_MIDI_ReceivePacket(pdev, (uint8_t)pdev->classId);
}

uint8_t USBD_MIDI_RegisterInterface(USBD_HandleTypeDef *pdev,
                                    USBD_MIDI_ItfTypeDef *fops)
{
    if(fops == NULL)
    {
        return USBD_FAIL;
    }
#ifdef USE_USBD_COMPOSITE
    pdev->pUserData[pdev->classId] = fops;
#else
    pdev->pUserData = fops;
#endif
    return USBD_OK;
}

uint8_t USBD_MIDI_SetTxBuffer(USBD_HandleTypeDef *pdev,
                              uint8_t *pbuff,
                              uint16_t length,
                              uint8_t class_id)
{
#ifdef USE_USBD_COMPOSITE
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)pdev->pClassDataCmsit[class_id];
#else
    (void)class_id;
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)pdev->pClassData;
#endif
    if(hmidi == NULL)
    {
        return USBD_FAIL;
    }
    hmidi->TxBuffer = pbuff;
    hmidi->TxLength = length;
    return USBD_OK;
}

uint8_t USBD_MIDI_SetRxBuffer(USBD_HandleTypeDef *pdev,
                              uint8_t *pbuff,
                              uint8_t class_id)
{
#ifdef USE_USBD_COMPOSITE
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)pdev->pClassDataCmsit[class_id];
#else
    (void)class_id;
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)pdev->pClassData;
#endif
    if(hmidi == NULL)
    {
        return USBD_FAIL;
    }
    hmidi->RxBuffer = pbuff;
    return USBD_OK;
}

uint8_t USBD_MIDI_TransmitPacket(USBD_HandleTypeDef *pdev, uint8_t class_id)
{
#ifdef USE_USBD_COMPOSITE
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)pdev->pClassDataCmsit[class_id];
    MIDIInEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_BULK, class_id);
#else
    (void)class_id;
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)pdev->pClassData;
#endif
    if(hmidi == NULL || hmidi->TxState != 0U)
    {
        return USBD_BUSY;
    }

    hmidi->TxState = 1U;
    pdev->ep_in[MIDIInEpAdd & 0xFU].total_length = hmidi->TxLength;
    return USBD_LL_Transmit(pdev, MIDIInEpAdd, hmidi->TxBuffer, hmidi->TxLength);
}

uint8_t USBD_MIDI_ReceivePacket(USBD_HandleTypeDef *pdev, uint8_t class_id)
{
#ifdef USE_USBD_COMPOSITE
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)pdev->pClassDataCmsit[class_id];
    MIDIOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_BULK, class_id);
#else
    (void)class_id;
    USBD_MIDI_HandleTypeDef *hmidi = (USBD_MIDI_HandleTypeDef *)pdev->pClassData;
#endif
    if(hmidi == NULL || hmidi->RxBuffer == NULL)
    {
        return USBD_FAIL;
    }

    return USBD_LL_PrepareReceive(pdev, MIDIOutEpAdd, hmidi->RxBuffer, MIDI_DATA_FS_OUT_PACKET_SIZE);
}
