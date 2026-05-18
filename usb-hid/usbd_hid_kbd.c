#include "usbd_hid.h"
#include "usbd_ctlreq.h"

static uint8_t USBD_HID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_HID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_HID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_HID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
#ifndef USE_USBD_COMPOSITE
static uint8_t *USBD_HID_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_HID_GetHSCfgDesc(uint16_t *length);
static uint8_t *USBD_HID_GetOtherSpeedCfgDesc(uint16_t *length);
static uint8_t *USBD_HID_GetDeviceQualifierDesc(uint16_t *length);
#endif

USBD_ClassTypeDef USBD_HID =
{
  USBD_HID_Init,
  USBD_HID_DeInit,
  USBD_HID_Setup,
  NULL,
  NULL,
  USBD_HID_DataIn,
  NULL,
  NULL,
  NULL,
  NULL,
#ifdef USE_USBD_COMPOSITE
  NULL,
  NULL,
  NULL,
  NULL,
#else
  USBD_HID_GetHSCfgDesc,
  USBD_HID_GetFSCfgDesc,
  USBD_HID_GetOtherSpeedCfgDesc,
  USBD_HID_GetDeviceQualifierDesc,
#endif
};

#ifndef USE_USBD_COMPOSITE
__ALIGN_BEGIN static uint8_t USBD_HID_CfgDesc[USB_HID_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09,
  USB_DESC_TYPE_CONFIGURATION,
  USB_HID_CONFIG_DESC_SIZ,
  0x00,
  0x01,
  0x01,
  0x00,
#if (USBD_SELF_POWERED == 1U)
  0xE0,
#else
  0xA0,
#endif
  USBD_MAX_POWER,

  0x09,
  USB_DESC_TYPE_INTERFACE,
  0x00,
  0x00,
  0x01,
  0x03,
  0x01,
  0x01,
  0x00,

  0x09,
  HID_DESCRIPTOR_TYPE,
  0x11,
  0x01,
  0x00,
  0x01,
  0x22,
  0x3F,
  0x00,

  0x07,
  USB_DESC_TYPE_ENDPOINT,
  HID_EPIN_ADDR,
  0x03,
  0x08,
  0x00,
  0x01,
};
#endif

__ALIGN_BEGIN static uint8_t USBD_HID_Desc[USB_HID_DESC_SIZ] __ALIGN_END =
{
  0x09,
  HID_DESCRIPTOR_TYPE,
  0x11,
  0x01,
  0x00,
  0x01,
  0x22,
  0x3F,
  0x00,
};

#ifndef USE_USBD_COMPOSITE
__ALIGN_BEGIN static uint8_t USBD_HID_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};
#endif

__ALIGN_BEGIN static uint8_t HID_KEYBOARD_ReportDesc[63] __ALIGN_END =
{
  0x05, 0x01,
  0x09, 0x06,
  0xA1, 0x01,
  0x05, 0x07,
  0x19, 0xE0,
  0x29, 0xE7,
  0x15, 0x00,
  0x25, 0x01,
  0x75, 0x01,
  0x95, 0x08,
  0x81, 0x02,
  0x95, 0x01,
  0x75, 0x08,
  0x81, 0x01,
  0x95, 0x05,
  0x75, 0x01,
  0x05, 0x08,
  0x19, 0x01,
  0x29, 0x05,
  0x91, 0x02,
  0x95, 0x01,
  0x75, 0x03,
  0x91, 0x01,
  0x95, 0x06,
  0x75, 0x08,
  0x15, 0x00,
  0x25, 0x65,
  0x05, 0x07,
  0x19, 0x00,
  0x29, 0x65,
  0x81, 0x00,
  0xC0
};

static uint8_t HIDInEpAdd = HID_EPIN_ADDR;

static uint8_t USBD_HID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);
  USBD_HID_HandleTypeDef *hhid;
  hhid = (USBD_HID_HandleTypeDef *)USBD_malloc(sizeof(USBD_HID_HandleTypeDef));
  if(hhid == NULL)
  {
    pdev->pClassDataCmsit[pdev->classId] = NULL;
    return (uint8_t)USBD_EMEM;
  }
  pdev->pClassDataCmsit[pdev->classId] = (void *)hhid;
  pdev->pClassData = pdev->pClassDataCmsit[pdev->classId];
#ifdef USE_USBD_COMPOSITE
  HIDInEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_INTR, (uint8_t)pdev->classId);
#endif
  pdev->ep_in[HIDInEpAdd & 0xFU].bInterval = (pdev->dev_speed == USBD_SPEED_HIGH) ? HID_HS_BINTERVAL : 0x01;
  (void)USBD_LL_OpenEP(pdev, HIDInEpAdd, USBD_EP_TYPE_INTR, 0x08);
  pdev->ep_in[HIDInEpAdd & 0xFU].is_used = 1U;
  hhid->state = USBD_HID_IDLE;
  return (uint8_t)USBD_OK;
}

static uint8_t USBD_HID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);
#ifdef USE_USBD_COMPOSITE
  HIDInEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_INTR, (uint8_t)pdev->classId);
#endif
  (void)USBD_LL_CloseEP(pdev, HIDInEpAdd);
  pdev->ep_in[HIDInEpAdd & 0xFU].is_used = 0U;
  pdev->ep_in[HIDInEpAdd & 0xFU].bInterval = 0U;
  if(pdev->pClassDataCmsit[pdev->classId] != NULL)
  {
    (void)USBD_free(pdev->pClassDataCmsit[pdev->classId]);
    pdev->pClassDataCmsit[pdev->classId] = NULL;
  }
  return (uint8_t)USBD_OK;
}

static uint8_t USBD_HID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
  USBD_StatusTypeDef ret = USBD_OK;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;
  if(hhid == NULL)
    return (uint8_t)USBD_FAIL;
  switch(req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS:
      switch(req->bRequest)
      {
        case USBD_HID_REQ_SET_PROTOCOL: hhid->Protocol = (uint8_t)(req->wValue); break;
        case USBD_HID_REQ_GET_PROTOCOL: (void)USBD_CtlSendData(pdev, (uint8_t *)&hhid->Protocol, 1U); break;
        case USBD_HID_REQ_SET_IDLE: hhid->IdleState = (uint8_t)(req->wValue >> 8); break;
        case USBD_HID_REQ_GET_IDLE: (void)USBD_CtlSendData(pdev, (uint8_t *)&hhid->IdleState, 1U); break;
        default: USBD_CtlError(pdev, req); ret = USBD_FAIL; break;
      }
      break;
    case USB_REQ_TYPE_STANDARD:
      switch(req->bRequest)
      {
        case USB_REQ_GET_STATUS:
          if(pdev->dev_state == USBD_STATE_CONFIGURED)
            (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
          else { USBD_CtlError(pdev, req); ret = USBD_FAIL; }
          break;
        case USB_REQ_GET_DESCRIPTOR:
          if((req->wValue >> 8) == HID_REPORT_DESC)
          {
            len = MIN(63U, req->wLength);
            pbuf = HID_KEYBOARD_ReportDesc;
          }
          else if((req->wValue >> 8) == HID_DESCRIPTOR_TYPE)
          {
            pbuf = USBD_HID_Desc;
            len = MIN(USB_HID_DESC_SIZ, req->wLength);
          }
          else { USBD_CtlError(pdev, req); ret = USBD_FAIL; break; }
          (void)USBD_CtlSendData(pdev, pbuf, len);
          break;
        case USB_REQ_GET_INTERFACE:
          if(pdev->dev_state == USBD_STATE_CONFIGURED)
            (void)USBD_CtlSendData(pdev, (uint8_t *)&hhid->AltSetting, 1U);
          else { USBD_CtlError(pdev, req); ret = USBD_FAIL; }
          break;
        case USB_REQ_SET_INTERFACE:
          if(pdev->dev_state == USBD_STATE_CONFIGURED)
            hhid->AltSetting = (uint8_t)(req->wValue);
          else { USBD_CtlError(pdev, req); ret = USBD_FAIL; }
          break;
        case USB_REQ_CLEAR_FEATURE:
          break;
        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;
    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
  }
  return (uint8_t)ret;
}

#ifdef USE_USBD_COMPOSITE
uint8_t USBD_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len, uint8_t ClassId)
{
  USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef *)pdev->pClassDataCmsit[ClassId];
#else
uint8_t USBD_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len)
{
  USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
#endif
  if(hhid == NULL)
    return (uint8_t)USBD_FAIL;
#ifdef USE_USBD_COMPOSITE
  HIDInEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_INTR, ClassId);
#endif
  if(pdev->dev_state == USBD_STATE_CONFIGURED)
  {
    if(hhid->state == USBD_HID_IDLE)
    {
      hhid->state = USBD_HID_BUSY;
      (void)USBD_LL_Transmit(pdev, HIDInEpAdd, report, len);
    }
  }
  return (uint8_t)USBD_OK;
}

uint32_t USBD_HID_GetPollingInterval(USBD_HandleTypeDef *pdev)
{
  return (pdev->dev_speed == USBD_SPEED_HIGH) ? (((1U << (HID_HS_BINTERVAL - 1U))) / 8U) : 0x01;
}

#ifndef USE_USBD_COMPOSITE
static uint8_t *USBD_HID_GetFSCfgDesc(uint16_t *length)
{
  USBD_EpDescTypeDef *pEpDesc = USBD_GetEpDesc(USBD_HID_CfgDesc, HID_EPIN_ADDR);
  if(pEpDesc != NULL)
    pEpDesc->bInterval = 0x01;
  *length = (uint16_t)sizeof(USBD_HID_CfgDesc);
  return USBD_HID_CfgDesc;
}

static uint8_t *USBD_HID_GetHSCfgDesc(uint16_t *length)
{
  USBD_EpDescTypeDef *pEpDesc = USBD_GetEpDesc(USBD_HID_CfgDesc, HID_EPIN_ADDR);
  if(pEpDesc != NULL)
    pEpDesc->bInterval = HID_HS_BINTERVAL;
  *length = (uint16_t)sizeof(USBD_HID_CfgDesc);
  return USBD_HID_CfgDesc;
}

static uint8_t *USBD_HID_GetOtherSpeedCfgDesc(uint16_t *length)
{
  USBD_EpDescTypeDef *pEpDesc = USBD_GetEpDesc(USBD_HID_CfgDesc, HID_EPIN_ADDR);
  if(pEpDesc != NULL)
    pEpDesc->bInterval = 0x01;
  *length = (uint16_t)sizeof(USBD_HID_CfgDesc);
  return USBD_HID_CfgDesc;
}

static uint8_t USBD_HID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  UNUSED(epnum);
  ((USBD_HID_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId])->state = USBD_HID_IDLE;
  return (uint8_t)USBD_OK;
}

static uint8_t *USBD_HID_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_HID_DeviceQualifierDesc);
  return USBD_HID_DeviceQualifierDesc;
}
#endif
