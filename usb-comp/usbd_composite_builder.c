/**
  ******************************************************************************
  * @file    usbd_composite_builder.c
  * @author  MCD Application Team
  * @brief   This file provides all the composite builder functions.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  * @verbatim
  *
  *          ===================================================================
  *                                Composite Builder  Description
  *          ===================================================================
  *
  *           The composite builder builds the configuration descriptors based on
  *           the selection of classes by user.
  *           It includes all USB Device classes in order to instantiate their
  *           descriptors, but for better management, it is possible to optimize
  *           footprint by removing unused classes. It is possible to do so by
  *           commenting the relative define in usbd_conf.h.
  *
  *  @endverbatim
  *
  ******************************************************************************
  */

/* BSPDependencies
- None
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_composite_builder.h"

#ifndef HID_KEYBOARD_REPORT_DESC_SIZE
#define HID_KEYBOARD_REPORT_DESC_SIZE 58U
#endif

#ifdef USE_USBD_COMPOSITE

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup CMPSIT_CORE
  * @brief Mass storage core module
  * @{
  */

/** @defgroup CMPSIT_CORE_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup CMPSIT_CORE_Private_Defines
  * @{
  */

/**
  * @}
  */


/** @defgroup CMPSIT_CORE_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup CMPSIT_CORE_Private_FunctionPrototypes
  * @{
  */
/* uint8_t  USBD_CMPSIT_Init (USBD_HandleTypeDef *pdev,
                            uint8_t cfgidx); */ /* Function not used for the moment */

/* uint8_t  USBD_CMPSIT_DeInit (USBD_HandleTypeDef *pdev,
                              uint8_t cfgidx); */ /* Function not used for the moment */

uint8_t  *USBD_CMPSIT_GetFSCfgDesc(uint16_t *length);
#ifdef USE_USB_HS
uint8_t  *USBD_CMPSIT_GetHSCfgDesc(uint16_t *length);
#endif /* USE_USB_HS */

uint8_t  *USBD_CMPSIT_GetOtherSpeedCfgDesc(uint16_t *length);

uint8_t  *USBD_CMPSIT_GetDeviceQualifierDescriptor(uint16_t *length);

static uint8_t USBD_CMPSIT_FindFreeIFNbr(USBD_HandleTypeDef *pdev);

static void  USBD_CMPSIT_AddConfDesc(uint32_t Conf, __IO uint32_t *pSze);

static void  USBD_CMPSIT_AssignEp(USBD_HandleTypeDef *pdev, uint8_t Add, uint8_t Type, uint32_t Sze);


#if USBD_CMPSIT_ACTIVATE_HID == 1U
static void  USBD_CMPSIT_HIDMouseDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_HID == 1U */

#if USBD_CMPSIT_ACTIVATE_MSC == 1U
static void  USBD_CMPSIT_MSCDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_MSC == 1U */

#if USBD_CMPSIT_ACTIVATE_CDC == 1U
static void  USBD_CMPSIT_CDCDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_CDC == 1U */

#if USBD_CMPSIT_ACTIVATE_DFU == 1U
static void  USBD_CMPSIT_DFUDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_DFU == 1U */

#if USBD_CMPSIT_ACTIVATE_RNDIS == 1U
static void  USBD_CMPSIT_RNDISDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_RNDIS == 1U */

#if USBD_CMPSIT_ACTIVATE_CDC_ECM == 1U
static void  USBD_CMPSIT_CDC_ECMDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_CDC_ECM == 1U */

#if USBD_CMPSIT_ACTIVATE_AUDIO == 1U
static void  USBD_CMPSIT_AUDIODesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_AUDIO == 1U */

#if USBD_CMPSIT_ACTIVATE_CUSTOMHID == 1
static void  USBD_CMPSIT_CUSTOMHIDDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_CUSTOMHID == 1U */

#if USBD_CMPSIT_ACTIVATE_VIDEO == 1U
static void  USBD_CMPSIT_VIDEODesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_VIDEO == 1U */

#if USBD_CMPSIT_ACTIVATE_PRINTER == 1U
static void  USBD_CMPSIT_PRNTDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_PRINTER == 1U */

#if USBD_CMPSIT_ACTIVATE_CCID == 1U
static void  USBD_CMPSIT_CCIDDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_CCID == 1U */

#if USBD_CMPSIT_ACTIVATE_MTP == 1U
static void  USBD_CMPSIT_MTPDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed);
#endif /* USBD_CMPSIT_ACTIVATE_MTP == 1U */

/**
  * @}
  */


/** @defgroup CMPSIT_CORE_Private_Variables
  * @{
  */
/* This structure is used only for the Configuration descriptors and Device Qualifier */
USBD_ClassTypeDef  USBD_CMPSIT =
{
  NULL, /* Init, */
  NULL, /* DeInit, */
  NULL, /* Setup, */
  NULL, /* EP0_TxSent, */
  NULL, /* EP0_RxReady, */
  NULL, /* DataIn, */
  NULL, /* DataOut, */
  NULL, /* SOF,  */
  NULL,
  NULL,
#ifdef USE_USB_HS
  USBD_CMPSIT_GetHSCfgDesc,
#else
  NULL,
#endif /* USE_USB_HS */
  USBD_CMPSIT_GetFSCfgDesc,
  USBD_CMPSIT_GetOtherSpeedCfgDesc,
  USBD_CMPSIT_GetDeviceQualifierDescriptor,
#if (USBD_SUPPORT_USER_STRING_DESC == 1U)
  NULL,
#endif /* USBD_SUPPORT_USER_STRING_DESC */
};

/* The generic configuration descriptor buffer that will be filled by builder
   Size of the buffer is the maximum possible configuration descriptor size. */
__ALIGN_BEGIN static uint8_t USBD_CMPSIT_FSCfgDesc[USBD_CMPST_MAX_CONFDESC_SZ]  __ALIGN_END = {0};
static uint8_t *pCmpstFSConfDesc = USBD_CMPSIT_FSCfgDesc;
/* Variable that dynamically holds the current size of the configuration descriptor */
static __IO uint32_t CurrFSConfDescSz = 0U;

#ifdef USE_USB_HS
__ALIGN_BEGIN static uint8_t USBD_CMPSIT_HSCfgDesc[USBD_CMPST_MAX_CONFDESC_SZ]  __ALIGN_END = {0};
static uint8_t *pCmpstHSConfDesc = USBD_CMPSIT_HSCfgDesc;
/* Variable that dynamically holds the current size of the configuration descriptor */
static __IO uint32_t CurrHSConfDescSz = 0U;
#endif /* USE_USB_HS */

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CMPSIT_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC]  __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,      /* bLength */
  USB_DESC_TYPE_DEVICE_QUALIFIER,  /* bDescriptorType */
  0x00,                            /* bcdDevice low */
  0x02,                            /* bcdDevice high */
  0xEF,                            /* Class */
  0x02,                            /* SubClass */
  0x01,                            /* Protocol */
  0x40,                            /* bMaxPacketSize0 */
  0x01,                            /* bNumConfigurations */
  0x00,                            /* bReserved */
};

/**
  * @}
  */


/** @defgroup CMPSIT_CORE_Private_Functions
  * @{
  */

/**
  * @brief  USBD_CMPSIT_AddClass
  *         Register a class in the class builder
  * @param  pdev: device instance
  * @param  pclass: pointer to the class structure to be added
  * @param  class: type of the class to be added (from USBD_CompositeClassTypeDef)
  * @param  cfgidx: configuration index
  * @retval status
  */
uint8_t  USBD_CMPSIT_AddClass(USBD_HandleTypeDef *pdev,
                              USBD_ClassTypeDef *pclass,
                              USBD_CompositeClassTypeDef class,
                              uint8_t cfgidx)
{
  if ((pdev->classId < USBD_MAX_SUPPORTED_CLASS) && (pdev->tclasslist[pdev->classId].Active == 0U))
  {
    /* Store the class parameters in the global tab */
    pdev->pClass[pdev->classId] = pclass;
    pdev->tclasslist[pdev->classId].ClassId = pdev->classId;
    pdev->tclasslist[pdev->classId].Active = 1U;
    pdev->tclasslist[pdev->classId].ClassType = class;

    /* Call configuration descriptor builder and endpoint configuration builder */
    if (USBD_CMPSIT_AddToConfDesc(pdev) != (uint8_t)USBD_OK)
    {
      return (uint8_t)USBD_FAIL;
    }
  }

  UNUSED(cfgidx);

  return (uint8_t)USBD_OK;
}


/**
  * @brief  USBD_CMPSIT_AddToConfDesc
  *         Add a new class to the configuration descriptor
  * @param  pdev: device instance
  * @retval status
  */
uint8_t  USBD_CMPSIT_AddToConfDesc(USBD_HandleTypeDef *pdev)
{
  uint8_t idxIf = 0U;
  uint8_t iEp = 0U;

  /* For the first class instance, start building the config descriptor common part */
  if (pdev->classId == 0U)
  {
    /* Add configuration and IAD descriptors */
    USBD_CMPSIT_AddConfDesc((uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz);
#ifdef USE_USB_HS
    USBD_CMPSIT_AddConfDesc((uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz);
#endif /* USE_USB_HS */
  }

  switch (pdev->tclasslist[pdev->classId].ClassType)
  {
#if USBD_CMPSIT_ACTIVATE_HID == 1
    case CLASS_TYPE_HID:
      /* Setup Max packet sizes (for HID, no dependency on USB Speed, both HS/FS have same packet size) */
      pdev->tclasslist[pdev->classId].CurrPcktSze = HID_EPIN_SIZE;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 1U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 1U; /* EP1_IN */

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];

      /* Assign IN Endpoint */
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_INTR, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_HIDMouseDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_HIDMouseDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_HID */

#if USBD_CMPSIT_ACTIVATE_MSC == 1
    case CLASS_TYPE_MSC:
      /* Setup default Max packet size */
      pdev->tclasslist[pdev->classId].CurrPcktSze = MSC_MAX_FS_PACKET;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 1U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 2U; /* EP1_IN, EP1_OUT */

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_MSCDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_MSCDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_MSC */

#if USBD_CMPSIT_ACTIVATE_CDC == 1
    case CLASS_TYPE_CDC:
      /* Setup default Max packet size for FS device */
      pdev->tclasslist[pdev->classId].CurrPcktSze = CDC_DATA_FS_MAX_PACKET_SIZE;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 2U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;
      pdev->tclasslist[pdev->classId].Ifs[1] = (uint8_t)(idxIf + 1U);

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 3U;

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set the second IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[2];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_CDCDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_CDCDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_CDC */

#if USBD_CMPSIT_ACTIVATE_DFU == 1
    case CLASS_TYPE_DFU:
      /* Setup Max packet sizes (for DFU, no dependency on USB Speed, both HS/FS have same packet size) */
      pdev->tclasslist[pdev->classId].CurrPcktSze = 64U;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 1U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 0U; /* only EP0 is used */

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_DFUDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_DFUDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_DFU */

#if USBD_CMPSIT_ACTIVATE_RNDIS == 1
    case CLASS_TYPE_RNDIS:
      /* Setup default Max packet size */
      pdev->tclasslist[pdev->classId].CurrPcktSze = CDC_RNDIS_DATA_FS_MAX_PACKET_SIZE;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 2U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;
      pdev->tclasslist[pdev->classId].Ifs[1] = (uint8_t)(idxIf + 1U);

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 3U;

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set the second IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[2];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_INTR, CDC_RNDIS_CMD_PACKET_SIZE);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_RNDISDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_RNDISDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_RNDIS */

#if USBD_CMPSIT_ACTIVATE_CDC_ECM == 1
    case CLASS_TYPE_ECM:
      /* Setup default Max packet size */
      pdev->tclasslist[pdev->classId].CurrPcktSze = CDC_ECM_DATA_FS_MAX_PACKET_SIZE;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 2U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;
      pdev->tclasslist[pdev->classId].Ifs[1] = (uint8_t)(idxIf + 1U);

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 3U; /* EP1_IN, EP1_OUT,CMD_EP2 */

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set the second IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[2];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_INTR, CDC_ECM_CMD_PACKET_SIZE);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_CDC_ECMDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_CDC_ECMDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_CDC_ECM */

#if USBD_CMPSIT_ACTIVATE_AUDIO == 1
    case CLASS_TYPE_AUDIO:
      /* Setup Max packet sizes. UAC1 stereo 16-bit at 48 kHz is 192 bytes/ms. */
      pdev->tclasslist[pdev->classId].CurrPcktSze = AUDIO_OUT_PACKET;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 3U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;
      pdev->tclasslist[pdev->classId].Ifs[1] = (uint8_t)(idxIf + 1U);
      pdev->tclasslist[pdev->classId].Ifs[2] = (uint8_t)(idxIf + 2U);

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 2U; /* OUT stream, IN stream */

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];

      /* Assign OUT Endpoint */
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_ISOC, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];

      /* Assign IN Endpoint */
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_ISOC, AUDIO_IN_PACKET_MAX);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_AUDIODesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_AUDIODesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_AUDIO */

#if USBD_CMPSIT_ACTIVATE_CUSTOMHID == 1
    case CLASS_TYPE_CHID:
      /* Setup Max packet sizes */
      pdev->tclasslist[pdev->classId].CurrPcktSze = CUSTOM_HID_EPOUT_SIZE;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 1U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 2U; /* EP1_IN, EP1_OUT */

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_INTR,CUSTOM_HID_EPIN_SIZE);

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_INTR, CUSTOM_HID_EPOUT_SIZE);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_CUSTOMHIDDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_CUSTOMHIDDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_CUSTOMHID */

#if USBD_CMPSIT_ACTIVATE_VIDEO == 1
    case CLASS_TYPE_VIDEO:
      /* Setup default Max packet size */
      pdev->tclasslist[pdev->classId].CurrPcktSze = UVC_ISO_FS_MPS;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 2U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;
      pdev->tclasslist[pdev->classId].Ifs[1] = (uint8_t)(idxIf + 1U);

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 1U; /* EP1_IN */

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];

      /* Assign IN Endpoint */
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_ISOC, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_VIDEODesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_VIDEODesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_VIDEO */

#if USBD_CMPSIT_ACTIVATE_PRINTER == 1
    case CLASS_TYPE_PRINTER:

      /* Setup default Max packet size */
      pdev->tclasslist[pdev->classId].CurrPcktSze = PRNT_DATA_FS_MAX_PACKET_SIZE;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 1U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 2U;

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_PRNTDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_PRNTDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_PRINTER */

#if USBD_CMPSIT_ACTIVATE_CCID == 1

    case CLASS_TYPE_CCID:
      /* Setup default Max packet size */
      pdev->tclasslist[pdev->classId].CurrPcktSze = CCID_DATA_FS_MAX_PACKET_SIZE;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 1U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 3U;

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set the second IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[2];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_INTR, CCID_CMD_PACKET_SIZE);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_CCIDDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_CCIDDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_CCID */

#if USBD_CMPSIT_ACTIVATE_MTP == 1

    case CLASS_TYPE_MTP:
      /* Setup default Max packet sizes */
      pdev->tclasslist[pdev->classId].CurrPcktSze = MTP_DATA_MAX_FS_PACKET_SIZE;

      /* Find the first available interface slot and Assign number of interfaces */
      idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);
      pdev->tclasslist[pdev->classId].NumIf = 1U;
      pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;

      /* Assign endpoint numbers */
      pdev->tclasslist[pdev->classId].NumEps = 3U;

      /* Set IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[0];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set OUT endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[1];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_BULK, pdev->tclasslist[pdev->classId].CurrPcktSze);

      /* Set the second IN endpoint slot */
      iEp = pdev->tclasslist[pdev->classId].EpAdd[2];
      USBD_CMPSIT_AssignEp(pdev, iEp, USBD_EP_TYPE_INTR, MTP_CMD_PACKET_SIZE);

      /* Configure and Append the Descriptor */
      USBD_CMPSIT_MTPDesc(pdev, (uint32_t)pCmpstFSConfDesc, &CurrFSConfDescSz, (uint8_t)USBD_SPEED_FULL);

#ifdef USE_USB_HS
      USBD_CMPSIT_MTPDesc(pdev, (uint32_t)pCmpstHSConfDesc, &CurrHSConfDescSz, (uint8_t)USBD_SPEED_HIGH);
#endif /* USE_USB_HS */

      break;
#endif /* USBD_CMPSIT_ACTIVATE_MTP */

    default:
      UNUSED(idxIf);
      UNUSED(iEp);
      UNUSED(USBD_CMPSIT_FindFreeIFNbr);
      UNUSED(USBD_CMPSIT_AssignEp);
      break;
  }

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_CMPSIT_GetFSCfgDesc
  *         return configuration descriptor for both FS and HS modes
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
uint8_t  *USBD_CMPSIT_GetFSCfgDesc(uint16_t *length)
{
  *length = (uint16_t)CurrFSConfDescSz;

  return USBD_CMPSIT_FSCfgDesc;
}

#ifdef USE_USB_HS
/**
  * @brief  USBD_CMPSIT_GetHSCfgDesc
  *         return configuration descriptor for both FS and HS modes
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
uint8_t  *USBD_CMPSIT_GetHSCfgDesc(uint16_t *length)
{
  *length = (uint16_t)CurrHSConfDescSz;

  return USBD_CMPSIT_HSCfgDesc;
}
#endif /* USE_USB_HS */

/**
  * @brief  USBD_CMPSIT_GetOtherSpeedCfgDesc
  *         return other speed configuration descriptor
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
uint8_t  *USBD_CMPSIT_GetOtherSpeedCfgDesc(uint16_t *length)
{
  *length = (uint16_t)CurrFSConfDescSz;

  return USBD_CMPSIT_FSCfgDesc;
}

/**
  * @brief  DeviceQualifierDescriptor
  *         return Device Qualifier descriptor
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
uint8_t  *USBD_CMPSIT_GetDeviceQualifierDescriptor(uint16_t *length)
{
  *length = (uint16_t)(sizeof(USBD_CMPSIT_DeviceQualifierDesc));
  return USBD_CMPSIT_DeviceQualifierDesc;
}

/**
  * @brief  USBD_CMPSIT_FindFreeIFNbr
  *         Find the first interface available slot
  * @param  pdev: device instance
  * @retval The interface number to be used
  */
static uint8_t USBD_CMPSIT_FindFreeIFNbr(USBD_HandleTypeDef *pdev)
{
  uint32_t idx = 0U;

  /* Unroll all already activated classes */
  for (uint32_t i = 0U; i < pdev->NumClasses; i++)
  {
    /* Unroll each class interfaces */
    for (uint32_t j = 0U; j < pdev->tclasslist[i].NumIf; j++)
    {
      /* Increment the interface counter index */
      idx++;
    }
  }

  /* Return the first available interface slot */
  return (uint8_t)idx;
}

/**
  * @brief  USBD_CMPSIT_AddToConfDesc
  *         Add a new class to the configuration descriptor
  * @param  pdev: device instance
  * @retval none
  */
static void  USBD_CMPSIT_AddConfDesc(uint32_t Conf, __IO uint32_t *pSze)
{
  /* Intermediate variable to comply with MISRA-C Rule 11.3 */
  USBD_ConfigDescTypeDef *ptr = (USBD_ConfigDescTypeDef *)Conf;

  ptr->bLength = (uint8_t)sizeof(USBD_ConfigDescTypeDef);
  ptr->bDescriptorType = USB_DESC_TYPE_CONFIGURATION;
  ptr->wTotalLength = 0U;
  ptr->bNumInterfaces = 0U;
  ptr->bConfigurationValue = 1U;
  ptr->iConfiguration = USBD_CONFIG_STR_DESC_IDX;

#if (USBD_SELF_POWERED == 1U)
  ptr->bmAttributes = 0xC0U;   /* bmAttributes: Self Powered according to user configuration */
#else
  ptr->bmAttributes = 0x80U;   /* bmAttributes: Bus Powered according to user configuration */
#endif /* USBD_SELF_POWERED */

  ptr->bMaxPower = USBD_MAX_POWER;

  *pSze += sizeof(USBD_ConfigDescTypeDef);
}

/**
  * @brief  USBD_CMPSIT_AssignEp
  *         Assign and endpoint
  * @param  pdev: device instance
  * @param  Add: Endpoint address
  * @param  Type: Endpoint type
  * @param  Sze: Endpoint max packet size
  * @retval none
  */
static void  USBD_CMPSIT_AssignEp(USBD_HandleTypeDef *pdev, uint8_t Add, uint8_t Type, uint32_t Sze)
{
  uint32_t idx = 0U;

  /* Find the first available endpoint slot */
  while (((idx < (pdev->tclasslist[pdev->classId]).NumEps) && \
          ((pdev->tclasslist[pdev->classId].Eps[idx].is_used) != 0U)))
  {
    /* Increment the index */
    idx++;
  }

  /* Configure the endpoint */
  pdev->tclasslist[pdev->classId].Eps[idx].add = Add;
  pdev->tclasslist[pdev->classId].Eps[idx].type = Type;
  pdev->tclasslist[pdev->classId].Eps[idx].size = (uint8_t)Sze;
  pdev->tclasslist[pdev->classId].Eps[idx].is_used = 1U;
}

#if USBD_CMPSIT_ACTIVATE_HID == 1
/**
  * @brief  USBD_CMPSIT_HIDMouseDesc
  *         Configure and Append the HID Mouse Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_HIDMouseDesc(USBD_HandleTypeDef *pdev, uint32_t pConf,
                                      __IO uint32_t *Sze, uint8_t speed)
{
  static USBD_IfDescTypeDef *pIfDesc;
  static USBD_EpDescTypeDef *pEpDesc;
  static USBD_HIDDescTypeDef *pHidMouseDesc;

  /* Append HID keyboard Interface descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0], 0U, \
                       (uint8_t)(pdev->tclasslist[pdev->classId].NumEps), 0x03U, 0x01U, 0x01U, 0U);

  /* Append HID Functional descriptor to Configuration descriptor */
  pHidMouseDesc = ((USBD_HIDDescTypeDef *)(pConf + *Sze));
  pHidMouseDesc->bLength = (uint8_t)sizeof(USBD_HIDDescTypeDef);
  pHidMouseDesc->bDescriptorType = HID_DESCRIPTOR_TYPE;
  pHidMouseDesc->bcdHID = 0x0111U;
  pHidMouseDesc->bCountryCode = 0x00U;
  pHidMouseDesc->bNumDescriptors = 0x01U;
  pHidMouseDesc->bHIDDescriptorType = 0x22U;
  pHidMouseDesc->wItemLength = HID_KEYBOARD_REPORT_DESC_SIZE;
  *Sze += (uint32_t)sizeof(USBD_HIDDescTypeDef);

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP(pdev->tclasslist[pdev->classId].Eps[0].add, USBD_EP_TYPE_INTR, HID_EPIN_SIZE, \
                       HID_HS_BINTERVAL, HID_FS_BINTERVAL);

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 1U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength  = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_HID == 1 */

#if USBD_CMPSIT_ACTIVATE_MSC == 1
/**
  * @brief  USBD_CMPSIT_MSCDesc
  *         Configure and Append the MSC Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_MSCDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  USBD_IfDescTypeDef *pIfDesc;
  USBD_EpDescTypeDef *pEpDesc;

  /* Append MSC Interface descriptor */
  __USBD_CMPSIT_SET_IF((pdev->tclasslist[pdev->classId].Ifs[0]), (0U), \
                       (uint8_t)(pdev->tclasslist[pdev->classId].NumEps), (0x08U), (0x06U), (0x50U), (0U));

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pdev->tclasslist[pdev->classId].CurrPcktSze = MSC_MAX_HS_PACKET;
  }

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[0].add), (USBD_EP_TYPE_BULK), \
                       (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[1].add), (USBD_EP_TYPE_BULK), \
                       (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 1U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_MSC == 1 */

#if USBD_CMPSIT_ACTIVATE_CDC == 1
/**
  * @brief  USBD_CMPSIT_MSCDesc
  *         Configure and Append the HID Mouse Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_CDCDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  static USBD_IfDescTypeDef               *pIfDesc;
  static USBD_EpDescTypeDef               *pEpDesc;
  static USBD_CDCHeaderFuncDescTypeDef    *pHeadDesc;
  static USBD_CDCCallMgmFuncDescTypeDef   *pCallMgmDesc;
  static USBD_CDCACMFuncDescTypeDef       *pACMDesc;
  static USBD_CDCUnionFuncDescTypeDef     *pUnionDesc;
#if USBD_COMPOSITE_USE_IAD == 1
  static USBD_IadDescTypeDef              *pIadDesc;
#endif /* USBD_COMPOSITE_USE_IAD == 1 */

#if USBD_COMPOSITE_USE_IAD == 1
  pIadDesc                          = ((USBD_IadDescTypeDef *)(pConf + *Sze));
  pIadDesc->bLength                 = (uint8_t)sizeof(USBD_IadDescTypeDef);
  pIadDesc->bDescriptorType         = USB_DESC_TYPE_IAD; /* IAD descriptor */
  pIadDesc->bFirstInterface         = pdev->tclasslist[pdev->classId].Ifs[0];
  pIadDesc->bInterfaceCount         = 2U;    /* 2 interfaces */
  pIadDesc->bFunctionClass          = 0x02U;
  pIadDesc->bFunctionSubClass       = 0x02U;
  pIadDesc->bFunctionProtocol       = 0x01U;
  pIadDesc->iFunction               = 0U; /* String Index */
  *Sze                              += (uint32_t)sizeof(USBD_IadDescTypeDef);
#endif /* USBD_COMPOSITE_USE_IAD == 1 */

  /* Control Interface Descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0], 0U, 1U, 0x02, 0x02U, 0x01U, 0U);

  /* Control interface headers */
  pHeadDesc = ((USBD_CDCHeaderFuncDescTypeDef *)((uint32_t)pConf + *Sze));
  /* Header Functional Descriptor*/
  pHeadDesc->bLength = 0x05U;
  pHeadDesc->bDescriptorType = 0x24U;
  pHeadDesc->bDescriptorSubtype = 0x00U;
  pHeadDesc->bcdCDC = 0x0110U;
  *Sze += (uint32_t)sizeof(USBD_CDCHeaderFuncDescTypeDef);

  /* Call Management Functional Descriptor */
  pCallMgmDesc = ((USBD_CDCCallMgmFuncDescTypeDef *)((uint32_t)pConf + *Sze));
  pCallMgmDesc->bLength = 0x05U;
  pCallMgmDesc->bDescriptorType = 0x24U;
  pCallMgmDesc->bDescriptorSubtype = 0x01U;
  pCallMgmDesc->bmCapabilities = 0x00U;
  pCallMgmDesc->bDataInterface = pdev->tclasslist[pdev->classId].Ifs[1];
  *Sze += (uint32_t)sizeof(USBD_CDCCallMgmFuncDescTypeDef);

  /* ACM Functional Descriptor*/
  pACMDesc = ((USBD_CDCACMFuncDescTypeDef *)((uint32_t)pConf + *Sze));
  pACMDesc->bLength = 0x04U;
  pACMDesc->bDescriptorType = 0x24U;
  pACMDesc->bDescriptorSubtype = 0x02U;
  pACMDesc->bmCapabilities = 0x02U;
  *Sze += (uint32_t)sizeof(USBD_CDCACMFuncDescTypeDef);

  /* Union Functional Descriptor*/
  pUnionDesc = ((USBD_CDCUnionFuncDescTypeDef *)((uint32_t)pConf + *Sze));
  pUnionDesc->bLength = 0x05U;
  pUnionDesc->bDescriptorType = 0x24U;
  pUnionDesc->bDescriptorSubtype = 0x06U;
  pUnionDesc->bMasterInterface = pdev->tclasslist[pdev->classId].Ifs[0];
  pUnionDesc->bSlaveInterface = pdev->tclasslist[pdev->classId].Ifs[1];
  *Sze += (uint32_t)sizeof(USBD_CDCUnionFuncDescTypeDef);

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP(pdev->tclasslist[pdev->classId].Eps[2].add, \
                       USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE, CDC_HS_BINTERVAL, CDC_FS_BINTERVAL);

  /* Data Interface Descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[1], 0U, 2U, 0x0A, 0U, 0U, 0U);

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pdev->tclasslist[pdev->classId].CurrPcktSze = CDC_DATA_HS_MAX_PACKET_SIZE;
  }

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[0].add), \
                       (USBD_EP_TYPE_BULK), (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[1].add), \
                       (USBD_EP_TYPE_BULK), (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 2U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_CDC == 1 */

#if USBD_CMPSIT_ACTIVATE_DFU == 1
/**
  * @brief  USBD_CMPSIT_DFUDesc
  *         Configure and Append the DFU Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_DFUDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  static USBD_IfDescTypeDef *pIfDesc;
  static USBD_DFUFuncDescTypeDef *pDFUFuncDesc;
  uint32_t idx;
  UNUSED(speed);

  for (idx = 0U; idx < USBD_DFU_MAX_ITF_NUM; idx++)
  {
    /* Append DFU Interface descriptor to Configuration descriptor */
    __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0], (uint8_t)idx, 0U, 0xFEU, 0x01U, 0x02U, \
                         (uint8_t)USBD_IDX_INTERFACE_STR + 1U + (uint8_t)idx);
  }

  /* Append DFU Functional descriptor to Configuration descriptor */
  pDFUFuncDesc = ((USBD_DFUFuncDescTypeDef *)(pConf + *Sze));
  pDFUFuncDesc->bLength              = (uint8_t)sizeof(USBD_DFUFuncDescTypeDef);
  pDFUFuncDesc->bDescriptorType      = DFU_DESCRIPTOR_TYPE;
  pDFUFuncDesc->bmAttributes         = USBD_DFU_BM_ATTRIBUTES;
  pDFUFuncDesc->wDetachTimeout       = USBD_DFU_DETACH_TIMEOUT;
  pDFUFuncDesc->wTransferSze         = USBD_DFU_XFER_SIZE;
  pDFUFuncDesc->bcdDFUVersion        = 0x011AU;
  *Sze                              += (uint32_t)sizeof(USBD_DFUFuncDescTypeDef);

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 1U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);

  UNUSED(idx);
}
#endif /* USBD_CMPSIT_ACTIVATE_DFU == 1 */

#if USBD_CMPSIT_ACTIVATE_CDC_ECM == 1
/**
  * @brief  USBD_CMPSIT_CDC_ECMDesc
  *         Configure and Append the CDC_ECM Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_CDC_ECMDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  static USBD_IfDescTypeDef             *pIfDesc;
  static USBD_EpDescTypeDef             *pEpDesc;
  static USBD_ECMFuncDescTypeDef        *pFuncDesc;
  static USBD_IadDescTypeDef            *pIadDesc;

  static USBD_CDCHeaderFuncDescTypeDef  *pHeadDesc;
  static USBD_CDCUnionFuncDescTypeDef   *pUnionDesc;

#if USBD_COMPOSITE_USE_IAD == 1
  pIadDesc                          = ((USBD_IadDescTypeDef *)(pConf + *Sze));
  pIadDesc->bLength                 = (uint8_t)sizeof(USBD_IadDescTypeDef);
  pIadDesc->bDescriptorType         = USB_DESC_TYPE_IAD; /* IAD descriptor */
  pIadDesc->bFirstInterface         = pdev->tclasslist[pdev->classId].Ifs[0];
  pIadDesc->bInterfaceCount         = 2U;    /* 2 interfaces */
  pIadDesc->bFunctionClass          = 0x02U;
  pIadDesc->bFunctionSubClass       = 0x06U;
  pIadDesc->bFunctionProtocol       = 0x00U;
  pIadDesc->iFunction               = 0U; /* String Index */
  *Sze                             += (uint32_t)sizeof(USBD_IadDescTypeDef);
#endif /* USBD_COMPOSITE_USE_IAD == 1 */

  /* Append ECM Interface descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0], 0U, 1U, 0x02U, 0x06U, 0U, 0U);

  /* Append ECM header functional descriptor to Configuration descriptor */
  pHeadDesc = ((USBD_CDCHeaderFuncDescTypeDef *)(pConf + *Sze));
  pHeadDesc->bLength                 = (uint8_t)sizeof(USBD_CDCHeaderFuncDescTypeDef);
  pHeadDesc->bDescriptorType         = USBD_FUNC_DESCRIPTOR_TYPE;
  pHeadDesc->bDescriptorSubtype      = 0x00U;
  pHeadDesc->bcdCDC                  = 0x1000U;
  *Sze += (uint32_t)sizeof(USBD_CDCHeaderFuncDescTypeDef);

  /* Append ECM functional descriptor to Configuration descriptor */
  pFuncDesc = ((USBD_ECMFuncDescTypeDef *)(pConf + *Sze));
  pFuncDesc->bFunctionLength         = (uint8_t)sizeof(USBD_ECMFuncDescTypeDef);
  pFuncDesc->bDescriptorType         = USBD_FUNC_DESCRIPTOR_TYPE;
  pFuncDesc->bDescriptorSubType      = USBD_DESC_SUBTYPE_ACM;
  pFuncDesc->iMacAddress             = CDC_ECM_MAC_STRING_INDEX;
  pFuncDesc->bEthernetStatistics3    = CDC_ECM_ETH_STATS_BYTE3;
  pFuncDesc->bEthernetStatistics2    = CDC_ECM_ETH_STATS_BYTE2;
  pFuncDesc->bEthernetStatistics1    = CDC_ECM_ETH_STATS_BYTE1;
  pFuncDesc->bEthernetStatistics0    = CDC_ECM_ETH_STATS_BYTE0;
  pFuncDesc->wMaxSegmentSize         = CDC_ECM_ETH_MAX_SEGSZE;
  pFuncDesc->bNumberMCFiltes         = CDC_ECM_ETH_NBR_MACFILTERS;
  pFuncDesc->bNumberPowerFiltes      = CDC_ECM_ETH_NBR_PWRFILTERS;
  *Sze += (uint32_t)sizeof(USBD_ECMFuncDescTypeDef);

  /* Append ECM Union functional descriptor to Configuration descriptor */
  pUnionDesc = ((USBD_CDCUnionFuncDescTypeDef *)(pConf + *Sze));
  pUnionDesc->bLength             = (uint8_t)sizeof(USBD_CDCUnionFuncDescTypeDef);
  pUnionDesc->bDescriptorType     = 0x24U;
  pUnionDesc->bDescriptorSubtype  = 0x06U;
  pUnionDesc->bMasterInterface    = pdev->tclasslist[pdev->classId].Ifs[0];
  pUnionDesc->bSlaveInterface     = pdev->tclasslist[pdev->classId].Ifs[1];
  *Sze += (uint32_t)sizeof(USBD_CDCUnionFuncDescTypeDef);

  /* Append ECM Communication IN Endpoint Descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP(pdev->tclasslist[pdev->classId].Eps[2].add, USBD_EP_TYPE_INTR, CDC_ECM_CMD_PACKET_SIZE, \
                       CDC_ECM_HS_BINTERVAL, CDC_ECM_FS_BINTERVAL);

  /* Append ECM Data class interface descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[1], 0U, 2U, 0x0AU, 0U, 0U, 0U);

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pdev->tclasslist[pdev->classId].CurrPcktSze = CDC_ECM_DATA_HS_MAX_PACKET_SIZE;
  }

  /* Append ECM OUT Endpoint Descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[0].add), (USBD_EP_TYPE_BULK), \
                       (pdev->tclasslist[pdev->classId].CurrPcktSze), (CDC_ECM_HS_BINTERVAL), (CDC_ECM_FS_BINTERVAL));

  /* Append ECM IN Endpoint Descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[1].add), (USBD_EP_TYPE_BULK), \
                       (pdev->tclasslist[pdev->classId].CurrPcktSze), (CDC_ECM_HS_BINTERVAL), (CDC_ECM_FS_BINTERVAL));

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 2U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_CDC_ECM */

#if USBD_CMPSIT_ACTIVATE_AUDIO == 1
/**
  * @brief  USBD_CMPSIT_AUDIODesc
  *         Configure and Append the AUDIO Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_AUDIODesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  uint8_t *desc = (uint8_t *)(pConf + *Sze);
  const uint8_t ac_if = pdev->tclasslist[pdev->classId].Ifs[0];
  const uint8_t out_if = pdev->tclasslist[pdev->classId].Ifs[1];
  const uint8_t in_if = pdev->tclasslist[pdev->classId].Ifs[2];
  const uint8_t out_ep = pdev->tclasslist[pdev->classId].Eps[0].add;
  const uint8_t in_ep = pdev->tclasslist[pdev->classId].Eps[1].add;
  uint32_t idx = 0U;
  UNUSED(speed);

#if USBD_COMPOSITE_USE_IAD == 1
  const uint8_t iad[] = {
    0x08U, USB_DESC_TYPE_IAD, ac_if, 0x03U,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOCONTROL, AUDIO_PROTOCOL_UNDEFINED, 0x00U,
  };
  USBD_memcpy(desc + idx, iad, sizeof(iad));
  idx += (uint32_t)sizeof(iad);
#endif /* USBD_COMPOSITE_USE_IAD == 1 */

  const uint8_t audio_desc[] = {
    /* AC interface */
    0x09U, USB_DESC_TYPE_INTERFACE, ac_if, 0x00U, 0x00U,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOCONTROL, AUDIO_PROTOCOL_UNDEFINED, 0x00U,

    /* AC header: playback and capture streaming interfaces */
    0x0AU, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_HEADER,
    0x00U, 0x01U, LOBYTE(0x0034U), HIBYTE(0x0034U),
    0x02U, out_if, in_if,

    /* USB OUT terminal pair: host -> Daisy playback */
    0x0CU, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_INPUT_TERMINAL,
    0x01U, 0x01U, 0x01U, 0x00U, 0x02U, 0x03U, 0x00U, 0x00U, 0x00U,
    0x09U, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_OUTPUT_TERMINAL,
    0x02U, 0x01U, 0x03U, 0x00U, 0x01U, 0x00U,

    /* USB IN terminal pair: Daisy capture -> host */
    0x0CU, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_INPUT_TERMINAL,
    0x03U, 0x01U, 0x02U, 0x00U, 0x02U, 0x03U, 0x00U, 0x00U, 0x00U,
    0x09U, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_CONTROL_OUTPUT_TERMINAL,
    0x04U, 0x01U, 0x01U, 0x00U, 0x03U, 0x00U,

    /* AS OUT interface */
    0x09U, USB_DESC_TYPE_INTERFACE, out_if, 0x00U, 0x00U,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00U,
    0x09U, USB_DESC_TYPE_INTERFACE, out_if, 0x01U, 0x01U,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00U,
    0x07U, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_GENERAL,
    0x01U, 0x01U, 0x01U, 0x00U,
    0x0BU, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_FORMAT_TYPE,
    AUDIO_FORMAT_TYPE_I, 0x02U, 0x02U, 16U, 0x01U, 0x80U, 0xBBU, 0x00U,
    0x09U, USB_DESC_TYPE_ENDPOINT, out_ep, 0x09U,
    LOBYTE(AUDIO_OUT_PACKET), HIBYTE(AUDIO_OUT_PACKET), 0x01U, 0x00U, 0x00U,
    0x07U, AUDIO_ENDPOINT_DESCRIPTOR_TYPE, AUDIO_ENDPOINT_GENERAL,
    0x01U, 0x00U, 0x00U, 0x00U,

    /* AS IN interface */
    0x09U, USB_DESC_TYPE_INTERFACE, in_if, 0x00U, 0x00U,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00U,
    0x09U, USB_DESC_TYPE_INTERFACE, in_if, 0x01U, 0x01U,
    USB_DEVICE_CLASS_AUDIO, AUDIO_SUBCLASS_AUDIOSTREAMING, AUDIO_PROTOCOL_UNDEFINED, 0x00U,
    0x07U, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_GENERAL,
    0x04U, 0x01U, 0x01U, 0x00U,
    0x0BU, AUDIO_INTERFACE_DESCRIPTOR_TYPE, AUDIO_STREAMING_FORMAT_TYPE,
    AUDIO_FORMAT_TYPE_I, 0x02U, 0x02U, 16U, 0x01U, 0x80U, 0xBBU, 0x00U,
    0x09U, USB_DESC_TYPE_ENDPOINT, in_ep, 0x05U,
    LOBYTE(AUDIO_IN_PACKET_MAX), HIBYTE(AUDIO_IN_PACKET_MAX), 0x01U, 0x00U, 0x00U,
    0x07U, AUDIO_ENDPOINT_DESCRIPTOR_TYPE, AUDIO_ENDPOINT_GENERAL,
    0x01U, 0x00U, 0x00U, 0x00U,
  };
  USBD_memcpy(desc + idx, audio_desc, sizeof(audio_desc));
  idx += (uint32_t)sizeof(audio_desc);
  *Sze += idx;

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 3U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_AUDIO */

#if USBD_CMPSIT_ACTIVATE_RNDIS == 1
/**
  * @brief  USBD_CMPSIT_MSCDesc
  *         Configure and Append the CDC_RNDIS Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_RNDISDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  static USBD_IfDescTypeDef               *pIfDesc;
  static USBD_EpDescTypeDef               *pEpDesc;
  static USBD_CDCHeaderFuncDescTypeDef    *pHeadDesc;
  static USBD_CDCCallMgmFuncDescTypeDef   *pCallMgmDesc;
  static USBD_CDCACMFuncDescTypeDef       *pACMDesc;
  static USBD_CDCUnionFuncDescTypeDef     *pUnionDesc;
  static USBD_IadDescTypeDef              *pIadDesc;

#if USBD_COMPOSITE_USE_IAD == 1
  pIadDesc                          = ((USBD_IadDescTypeDef *)(pConf + *Sze));
  pIadDesc->bLength                 = (uint8_t)sizeof(USBD_IadDescTypeDef);
  pIadDesc->bDescriptorType         = USB_DESC_TYPE_IAD; /* IAD descriptor */
  pIadDesc->bFirstInterface         = pdev->tclasslist[pdev->classId].Ifs[0];
  pIadDesc->bInterfaceCount         = 2U;    /* 2 interfaces */
  pIadDesc->bFunctionClass          = 0xE0U;
  pIadDesc->bFunctionSubClass       = 0x01U;
  pIadDesc->bFunctionProtocol       = 0x03U;
  pIadDesc->iFunction               = 0U; /* String Index */
  *Sze                              += (uint32_t)sizeof(USBD_IadDescTypeDef);
#endif /* USBD_COMPOSITE_USE_IAD == 1 */

  /* Control Interface Descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0], 0U, 1U, 0x02, 0x02, 0xFF, 0U);

  /* Control interface headers */
  pHeadDesc = ((USBD_CDCHeaderFuncDescTypeDef *)(pConf + *Sze));
  /* Header Functional Descriptor*/
  pHeadDesc->bLength            = (uint8_t)sizeof(USBD_CDCHeaderFuncDescTypeDef);
  pHeadDesc->bDescriptorType    = 0x24U;
  pHeadDesc->bDescriptorSubtype = 0x00U;
  pHeadDesc->bcdCDC             = 0x0110U;
  *Sze += (uint32_t)sizeof(USBD_CDCHeaderFuncDescTypeDef);

  /* Call Management Functional Descriptor*/
  pCallMgmDesc                     = ((USBD_CDCCallMgmFuncDescTypeDef *)(pConf + *Sze));
  pCallMgmDesc->bLength            = (uint8_t)sizeof(USBD_CDCCallMgmFuncDescTypeDef);
  pCallMgmDesc->bDescriptorType    = 0x24U;
  pCallMgmDesc->bDescriptorSubtype = 0x01U;
  pCallMgmDesc->bmCapabilities     = 0x00U;
  pCallMgmDesc->bDataInterface     = pdev->tclasslist[pdev->classId].Ifs[1];
  *Sze += (uint32_t)sizeof(USBD_CDCCallMgmFuncDescTypeDef);

  /* ACM Functional Descriptor*/
  pACMDesc                      = ((USBD_CDCACMFuncDescTypeDef *)(pConf + *Sze));
  pACMDesc->bLength             = (uint8_t)sizeof(USBD_CDCACMFuncDescTypeDef);
  pACMDesc->bDescriptorType     = 0x24U;
  pACMDesc->bDescriptorSubtype  = 0x02U;
  pACMDesc->bmCapabilities      = 0x00U;
  *Sze += (uint32_t)sizeof(USBD_CDCACMFuncDescTypeDef);

  /* Union Functional Descriptor*/
  pUnionDesc                      = ((USBD_CDCUnionFuncDescTypeDef *)(pConf + *Sze));
  pUnionDesc->bLength             = (uint8_t)sizeof(USBD_CDCUnionFuncDescTypeDef);
  pUnionDesc->bDescriptorType     = 0x24U;
  pUnionDesc->bDescriptorSubtype  = 0x06U;
  pUnionDesc->bMasterInterface    = pdev->tclasslist[pdev->classId].Ifs[0];
  pUnionDesc->bSlaveInterface     = pdev->tclasslist[pdev->classId].Ifs[1];
  *Sze += (uint32_t)sizeof(USBD_CDCUnionFuncDescTypeDef);

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP(pdev->tclasslist[pdev->classId].Eps[2].add, USBD_EP_TYPE_INTR, \
                       CDC_RNDIS_CMD_PACKET_SIZE, CDC_RNDIS_HS_BINTERVAL, CDC_RNDIS_FS_BINTERVAL);

  /* Data Interface Descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[1], 0U, 2U, 0x0AU, 0x00U, 0x00U, 0U);

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pdev->tclasslist[pdev->classId].CurrPcktSze = CDC_RNDIS_DATA_HS_MAX_PACKET_SIZE;
  }

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[0].add), (USBD_EP_TYPE_BULK), \
                       (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[1].add), (USBD_EP_TYPE_BULK), \
                       (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 2U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_RNDIS == 1 */

#if USBD_CMPSIT_ACTIVATE_CUSTOMHID == 1
/**
  * @brief  USBD_CMPSIT_CUSTOMHIDDesc
  *         Configure and Append the MSC Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_CUSTOMHIDDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  static USBD_IfDescTypeDef *pIfDesc;
  static USBD_EpDescTypeDef *pEpDesc;
  static USBD_DescTypeDef *pDesc;

  /* Control Interface Descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0],  0U, 2U, 3U, 0U, 0U, 0U);

  /* Descriptor of CUSTOM_HID */
  pDesc = ((USBD_DescTypeDef *)((uint32_t)pConf + *Sze));
  pDesc->bLength = 0x09U;
  pDesc->bDescriptorTypeCHID = CUSTOM_HID_DESCRIPTOR_TYPE;
  pDesc->bcdCUSTOM_HID = 0x0111U;
  pDesc->bCountryCode = 0x00U;
  pDesc->bNumDescriptors = 0x01U;
  pDesc->bDescriptorType = 0x22U;
#ifdef USBD_CUSTOMHID_REPORT_DESC_SIZE_ENABLED
  pDesc->wItemLength = ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData[pdev->classId])->wReportDescLen;
#else
  pDesc->wItemLength = USBD_CUSTOM_HID_REPORT_DESC_SIZE;
#endif /* USBD_CUSTOMHID_REPORT_DESC_SIZE_ENABLED */

  *Sze += (uint32_t)sizeof(USBD_DescTypeDef);

  /* Descriptor of Custom HID endpoints */
  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP(pdev->tclasslist[pdev->classId].Eps[0].add, \
                       USBD_EP_TYPE_INTR, CUSTOM_HID_EPIN_SIZE, CUSTOM_HID_HS_BINTERVAL, CUSTOM_HID_FS_BINTERVAL);

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP(pdev->tclasslist[pdev->classId].Eps[1].add, \
                       USBD_EP_TYPE_INTR, CUSTOM_HID_EPOUT_SIZE, CUSTOM_HID_HS_BINTERVAL, CUSTOM_HID_FS_BINTERVAL);

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 1U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_CUSTOMHID == 1U */

#if USBD_CMPSIT_ACTIVATE_VIDEO == 1
/**
  * @brief  USBD_CMPSIT_VIDEODesc
  *         Configure and Append the VIDEO Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_VIDEODesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
#ifdef USBD_UVC_FORMAT_UNCOMPRESSED
  __ALIGN_BEGIN static uint8_t usbd_uvc_guid[16] __ALIGN_END = {DBVAL(UVC_UNCOMPRESSED_GUID), 0x00, 0x00, 0x10,
                                                                0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
                                                               };
#endif /* USBD_UVC_FORMAT_UNCOMPRESSED */
  static USBD_IfDescTypeDef *pIfDesc;
  static USBD_IadDescTypeDef *pIadDesc;

  /* Append AUDIO Interface descriptor to Configuration descriptor */
  USBD_specificVCInDescTypeDef            *pSVCInDesc;
  USBD_InputTerminalDescTypeDef           *pInTerDesc;
  USBD_OutputTerminalDescTypeDef          *pOuTerDesc;
  USBD_ClassSpecificVsHeaderDescTypeDef   *pSpHeaDesc;
  USBD_PayloadFormatDescTypeDef           *pPayForDesc;
#ifdef USBD_UVC_FORMAT_UNCOMPRESSED
  USBD_ColorMatchingDescTypeDef           *pColMaDesc;
#endif /* USBD_UVC_FORMAT_UNCOMPRESSED */
  USBD_StandardVCDataEPDescTypeDef        *pSVCDEP;
  USBD_VIDEO_VSFrameDescTypeDef           *pClassSpecVS;

#if USBD_COMPOSITE_USE_IAD == 1
  pIadDesc                          = ((USBD_IadDescTypeDef *)(pConf + *Sze));
  pIadDesc->bLength                 = (uint8_t)sizeof(USBD_IadDescTypeDef);
  pIadDesc->bDescriptorType         = USB_DESC_TYPE_IAD; /* IAD descriptor */
  pIadDesc->bFirstInterface         = pdev->tclasslist[pdev->classId].Ifs[0];
  pIadDesc->bInterfaceCount         = 2U;    /* 2 interfaces */
  pIadDesc->bFunctionClass          = UVC_CC_VIDEO;
  pIadDesc->bFunctionSubClass       = SC_VIDEO_INTERFACE_COLLECTION;
  pIadDesc->bFunctionProtocol       = PC_PROTOCOL_UNDEFINED;
  pIadDesc->iFunction               = 0U; /* String Index */
  *Sze                             += (uint32_t)sizeof(USBD_IadDescTypeDef);
#endif /* USBD_COMPOSITE_USE_IAD == 1 */

  /* Append VIDEO Interface descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0], 0U, 0U, UVC_CC_VIDEO, 1U, PC_PROTOCOL_UNDEFINED, 0U);

  /* Append Class-specific VC Interface Descriptor to Configuration descriptor*/
  pSVCInDesc = ((USBD_specificVCInDescTypeDef *)(pConf + *Sze));
  pSVCInDesc->bLength = (uint8_t)sizeof(USBD_specificVCInDescTypeDef);
  pSVCInDesc->bDescriptorType = CS_INTERFACE;
  pSVCInDesc->bDescriptorSubtype = VC_HEADER;
  pSVCInDesc->bcdUVC = UVC_VERSION;
  pSVCInDesc->wTotalLength = 0x001EU;
  pSVCInDesc->dwClockFrequency = 0x02DC6C00U;
  pSVCInDesc->baInterfaceNr = 0x01U;
  pSVCInDesc->iTerminal = 0x01U;
  *Sze += (uint32_t)sizeof(USBD_specificVCInDescTypeDef);

  /*Append Input Terminal Descriptor to Configuration descriptor */
  pInTerDesc = ((USBD_InputTerminalDescTypeDef *)(pConf + *Sze));
  pInTerDesc->bLength = (uint8_t)sizeof(USBD_InputTerminalDescTypeDef);
  pInTerDesc->bDescriptorType = CS_INTERFACE;
  pInTerDesc->bDescriptorSubtype = VC_INPUT_TERMINAL;
  pInTerDesc->bTerminalID = 0x01U;
  pInTerDesc->wTerminalType = ITT_VENDOR_SPECIFIC;
  pInTerDesc->bAssocTerminal = 0x00U;
  pInTerDesc->iTerminal =  0x00U;
  *Sze += (uint32_t)sizeof(USBD_InputTerminalDescTypeDef);

  /* Append Output Terminal Descriptor to Configuration descriptor */
  pOuTerDesc = ((USBD_OutputTerminalDescTypeDef *)(pConf + *Sze));
  pOuTerDesc->bLength = (uint8_t)sizeof(USBD_OutputTerminalDescTypeDef);
  pOuTerDesc->bDescriptorType = CS_INTERFACE;
  pOuTerDesc->bDescriptorSubtype = VC_OUTPUT_TERMINAL;
  pOuTerDesc->bTerminalID = 0x02U;
  pOuTerDesc->wTerminalType = TT_STREAMING;
  pOuTerDesc->bAssocTerminal = 0x00U;
  pOuTerDesc->bSourceID = 0x01U;
  pOuTerDesc->iTerminal = 0x00U;
  *Sze += (uint32_t)sizeof(USBD_OutputTerminalDescTypeDef);

  /* Standard VS (Video Streaming) Interface Descriptor */
  /* Interface 1, Alternate Setting 0 = Zero Bandwidth*/
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[1], 0U, 0U, UVC_CC_VIDEO, \
                       SC_VIDEOSTREAMING, PC_PROTOCOL_UNDEFINED, 0U);

  /* Append Class-specific VS Header Descriptor (Input) to Configuration descriptor */
  pSpHeaDesc = ((USBD_ClassSpecificVsHeaderDescTypeDef *)(pConf + *Sze));
  pSpHeaDesc->bLength = (uint8_t)sizeof(USBD_ClassSpecificVsHeaderDescTypeDef);
  pSpHeaDesc->bDescriptorType = CS_INTERFACE;
  pSpHeaDesc->bDescriptorSubtype = VS_INPUT_HEADER;
  pSpHeaDesc->bNumFormats = 0x4D01U;
  pSpHeaDesc->bVideoControlSize = 0x00U;
  pSpHeaDesc->bEndPointAddress = UVC_IN_EP;
  pSpHeaDesc->bmInfo = 0x00U;
  pSpHeaDesc->bTerminalLink = 0x02U;
  pSpHeaDesc->bStillCaptureMethod = 0x00U;
  pSpHeaDesc->bTriggerSupport = 0x00U;
  pSpHeaDesc->bTriggerUsage = 0x00U;
  pSpHeaDesc->bControlSize = 0x01U;
  pSpHeaDesc->bmaControls = 0x00U;
  *Sze += (uint32_t)sizeof(USBD_ClassSpecificVsHeaderDescTypeDef);

  /* Append Payload Format Descriptor to Configuration descriptor */
  pPayForDesc = ((USBD_PayloadFormatDescTypeDef *)(pConf + *Sze));
  pPayForDesc->bLength = (uint8_t)sizeof(USBD_PayloadFormatDescTypeDef);
  pPayForDesc->bDescriptorType = CS_INTERFACE;
  pPayForDesc->bDescriptorSubType = VS_FORMAT_SUBTYPE;
  pPayForDesc->bFormatIndex = 0x01U;
  pPayForDesc->bNumFrameDescriptor = 0x01U;
#ifdef USBD_UVC_FORMAT_UNCOMPRESSED
  (void)USBD_memcpy(pPayForDesc->pGiudFormat, usbd_uvc_guid, 16);
  pPayForDesc->bBitsPerPixel = UVC_BITS_PER_PIXEL;
#else
  pPayForDesc->bmFlags = 0x01U;
#endif /* USBD_UVC_FORMAT_UNCOMPRESSED */
  pPayForDesc->bDefaultFrameIndex = 0x01U;
  pPayForDesc->bAspectRatioX = 0x00U;
  pPayForDesc->bAspectRatioY = 0x00U;
  pPayForDesc->bInterlaceFlags = 0x00U;
  pPayForDesc->bCopyProtect = 0x00U;
  *Sze += (uint32_t)sizeof(USBD_PayloadFormatDescTypeDef);

  /* Append Class-specific VS (Video Streaming) Frame Descriptor to Configuration descriptor */
  pClassSpecVS = ((USBD_VIDEO_VSFrameDescTypeDef *)(pConf + *Sze));
  pClassSpecVS->bLength = (uint8_t)sizeof(USBD_VIDEO_VSFrameDescTypeDef);
  pClassSpecVS->bDescriptorType = CS_INTERFACE;
  pClassSpecVS->bDescriptorSubType = VS_FRAME_SUBTYPE;
  pClassSpecVS->bFrameIndex = 0x01U;

#ifdef USBD_UVC_FORMAT_UNCOMPRESSED
  pClassSpecVS->bmCapabilities = 0x00U;
#else
  pClassSpecVS->bmCapabilities = 0x02U;
#endif /* USBD_UVC_FORMAT_UNCOMPRESSED */

  pClassSpecVS->wWidth = UVC_WIDTH;
  pClassSpecVS->wHeight = UVC_HEIGHT;

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pClassSpecVS->dwMinBitRate = UVC_MIN_BIT_RATE(UVC_CAM_FPS_HS);
    pClassSpecVS->dwMaxBitRate = UVC_MAX_BIT_RATE(UVC_CAM_FPS_HS);
    pClassSpecVS->dwDefaultFrameInterval = UVC_INTERVAL(UVC_CAM_FPS_HS);
    pClassSpecVS->dwMinFrameInterval = UVC_INTERVAL(UVC_CAM_FPS_HS);
  }
  else
  {
    pClassSpecVS->dwMinBitRate = UVC_MIN_BIT_RATE(UVC_CAM_FPS_FS);
    pClassSpecVS->dwMaxBitRate = UVC_MAX_BIT_RATE(UVC_CAM_FPS_FS);
    pClassSpecVS->dwDefaultFrameInterval = UVC_INTERVAL(UVC_CAM_FPS_FS);
    pClassSpecVS->dwMinFrameInterval = UVC_INTERVAL(UVC_CAM_FPS_FS);
  }

  pClassSpecVS->dwMaxVideoFrameBufSize = UVC_MAX_FRAME_SIZE;
  pClassSpecVS->bFrameIntervalType = 0x01U;

  *Sze += (uint32_t)sizeof(USBD_VIDEO_VSFrameDescTypeDef);

#ifdef USBD_UVC_FORMAT_UNCOMPRESSED
  /* Append Color Matching Descriptor to Configuration descriptor */
  pColMaDesc = ((USBD_ColorMatchingDescTypeDef *)(pConf + *Sze));
  pColMaDesc->bLength = (uint8_t)sizeof(USBD_ColorMatchingDescTypeDef);
  pColMaDesc->bDescriptorType = CS_INTERFACE;
  pColMaDesc->bDescriptorSubType = VS_COLORFORMAT;
  pColMaDesc->bColorPrimarie = UVC_COLOR_PRIMARIE;
  pColMaDesc->bTransferCharacteristics = UVC_TFR_CHARACTERISTICS;
  pColMaDesc->bMatrixCoefficients = UVC_MATRIX_COEFFICIENTS;
  *Sze += (uint32_t)sizeof(USBD_ColorMatchingDescTypeDef);
#endif /* USBD_UVC_FORMAT_UNCOMPRESSED */

  /* USB Standard VS Interface  Descriptor - data transfer mode */
  /* Interface 1, Alternate Setting 1*/
  __USBD_CMPSIT_SET_IF(1U, 1U, 1U, UVC_CC_VIDEO, SC_VIDEOSTREAMING, PC_PROTOCOL_UNDEFINED, 0U);

  /* Standard VS (Video Streaming) data Endpoint */
  pSVCDEP = ((USBD_StandardVCDataEPDescTypeDef *)(pConf + *Sze));
  pSVCDEP->bLength = (uint8_t)sizeof(USBD_StandardVCDataEPDescTypeDef);
  pSVCDEP->bDescriptorType = USB_DESC_TYPE_ENDPOINT;
  pSVCDEP->bEndpointAddress = UVC_IN_EP;
  pSVCDEP->bmAttributes = 0x05U;
  pSVCDEP->bInterval = 0x01U;

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pSVCDEP->wMaxPacketSize = UVC_ISO_HS_MPS;
  }
  else
  {
    pSVCDEP->wMaxPacketSize = UVC_ISO_FS_MPS;
  }

  *Sze += (uint32_t)sizeof(USBD_StandardVCDataEPDescTypeDef);

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 2U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_VIDEO == 1 */

#if USBD_CMPSIT_ACTIVATE_PRINTER == 1
/**
  * @brief  USBD_CMPSIT_PRINTERDesc
  *         Configure and Append the PRINTER Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_PRNTDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  static USBD_IfDescTypeDef *pIfDesc;
  static USBD_EpDescTypeDef *pEpDesc;

  /* Control Interface Descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0], 0U, 0x02, 0x07, 0x01U, USB_PRNT_BIDIRECTIONAL, 0U);

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pdev->tclasslist[pdev->classId].CurrPcktSze = PRNT_DATA_HS_MAX_PACKET_SIZE;
  }

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[1].add), \
                       (USBD_EP_TYPE_BULK), (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[0].add), \
                       (USBD_EP_TYPE_BULK), (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 1U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_PRINTER == 1 */

#if USBD_CMPSIT_ACTIVATE_CCID == 1
/**
  * @brief  USBD_CMPSIT_CCIDDesc
  *         Configure and Append the CCID Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_CCIDDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  static USBD_IfDescTypeDef *pIfDesc;
  static USBD_EpDescTypeDef *pEpDesc;
  static USBD_CCID_DescTypeDef *pDesc;

  /* Control Interface Descriptor */
  __USBD_CMPSIT_SET_IF(pdev->tclasslist[pdev->classId].Ifs[0], 0U, 0x03, 0x0BU, 0U, 0U, 0U);

  /* Control interface headers */
  pDesc = ((USBD_CCID_DescTypeDef *)((uint32_t)pConf + *Sze));

  /* Device Descriptor */
  pDesc->bLength = 0x36U;
  pDesc->bDescriptorType = 0x21U;
  pDesc->bcdCCID = 0x0110U;
  pDesc->bMaxSlotIndex = 0x00U;
  pDesc->bVoltageSupport = CCID_VOLTAGE_SUPP;
  pDesc->dwProtocols = USBD_CCID_PROTOCOL;
  pDesc->dwDefaultClock = USBD_CCID_DEFAULT_CLOCK_FREQ;
  pDesc->dwMaximumClock = USBD_CCID_MAX_CLOCK_FREQ;
  pDesc->bNumClockSupported = 0x00U;
  pDesc->dwDataRate = USBD_CCID_DEFAULT_DATA_RATE;
  pDesc->dwMaxDataRate = USBD_CCID_MAX_DATA_RATE;
  pDesc->bNumDataRatesSupported = 0x35U;
  pDesc->dwMaxIFSD = USBD_CCID_MAX_INF_FIELD_SIZE;
  pDesc->dwSynchProtocols = 0U;
  pDesc->dwMechanical = 0U;
  pDesc->dwFeatures = 0x000104BAU;
  pDesc->dwMaxCCIDMessageLength = CCID_MAX_BLOCK_SIZE_HEADER;
  pDesc->bClassGetResponse = 0U;
  pDesc->bClassEnvelope = 0U;
  pDesc->wLcdLayout = 0U;
  pDesc->bPINSupport = 0x03U;
  pDesc->bMaxCCIDBusySlots = 0x01U;

  *Sze += (uint32_t)sizeof(USBD_CCID_DescTypeDef);

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pdev->tclasslist[pdev->classId].CurrPcktSze = CCID_DATA_HS_MAX_PACKET_SIZE;
  }

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[0].add), \
                       (USBD_EP_TYPE_BULK), (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[1].add), \
                       (USBD_EP_TYPE_BULK), (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP(pdev->tclasslist[pdev->classId].Eps[2].add, \
                       USBD_EP_TYPE_INTR, CCID_CMD_PACKET_SIZE, CCID_CMD_HS_BINTERVAL, CCID_CMD_FS_BINTERVAL);

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 1U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_CCID == 1 */

#if USBD_CMPSIT_ACTIVATE_MTP == 1
/**
  * @brief  USBD_CMPSIT_MTPDesc
  *         Configure and Append the MTP Descriptor
  * @param  pdev: device instance
  * @param  pConf: Configuration descriptor pointer
  * @param  Sze: pointer to the current configuration descriptor size
  * @retval None
  */
static void  USBD_CMPSIT_MTPDesc(USBD_HandleTypeDef *pdev, uint32_t pConf, __IO uint32_t *Sze, uint8_t speed)
{
  USBD_IfDescTypeDef *pIfDesc;
  USBD_EpDescTypeDef *pEpDesc;

  /* Append MTP Interface descriptor */
  __USBD_CMPSIT_SET_IF((pdev->tclasslist[pdev->classId].Ifs[0]), (0U), \
                       (uint8_t)(pdev->tclasslist[pdev->classId].NumEps), USB_MTP_INTRERFACE_CLASS, \
                       USB_MTP_INTRERFACE_SUB_CLASS, USB_MTP_INTRERFACE_PROTOCOL, (0U));

  if (speed == (uint8_t)USBD_SPEED_HIGH)
  {
    pdev->tclasslist[pdev->classId].CurrPcktSze = MTP_DATA_MAX_HS_PACKET_SIZE;
  }

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[0].add), (USBD_EP_TYPE_BULK), \
                       (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP((pdev->tclasslist[pdev->classId].Eps[1].add), (USBD_EP_TYPE_BULK), \
                       (pdev->tclasslist[pdev->classId].CurrPcktSze), (0U), (0U));

  /* Append Endpoint descriptor to Configuration descriptor */
  __USBD_CMPSIT_SET_EP(pdev->tclasslist[pdev->classId].Eps[2].add, \
                       USBD_EP_TYPE_INTR, MTP_CMD_PACKET_SIZE, MTP_HS_BINTERVAL, MTP_FS_BINTERVAL);

  /* Update Config Descriptor and IAD descriptor */
  ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 1U;
  ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)(*Sze);
}
#endif /* USBD_CMPSIT_ACTIVATE_MTP == 1 */

/**
  * @brief  USBD_CMPSIT_SetClassID
  *         Find and set the class ID relative to selected class type and instance
  * @param  pdev: device instance
  * @param  Class: Class type, can be CLASS_TYPE_NONE if requested to find class from setup request
  * @param  Instance: Instance number of the class (0 if first/unique instance, >0 otherwise)
  * @retval The Class ID, The pdev->classId is set with the value of the selected class ID.
  */
uint32_t  USBD_CMPSIT_SetClassID(USBD_HandleTypeDef *pdev, USBD_CompositeClassTypeDef Class, uint32_t Instance)
{
  uint32_t idx;
  uint32_t inst = 0U;

  /* Unroll all already activated classes */
  for (idx = 0U; idx < pdev->NumClasses; idx++)
  {
    /* Check if the class correspond to the requested type and if it is active */
    if (((USBD_CompositeClassTypeDef)(pdev->tclasslist[idx].ClassType) == Class) &&
        ((pdev->tclasslist[idx].Active) == 1U))
    {
      if (inst == Instance)
      {
        /* Set the new class ID */
        pdev->classId = idx;

        /* Return the class ID value */
        return (idx);
      }
      else
      {
        /* Increment instance index and look for next instance */
        inst++;
      }
    }
  }

  /* No class found, return 0xFF */
  return 0xFFU;
}

/**
  * @brief  USBD_CMPSIT_GetClassID
  *         Returns the class ID relative to selected class type and instance
  * @param  pdev: device instance
  * @param  Class: Class type, can be CLASS_TYPE_NONE if requested to find class from setup request
  * @param  Instance: Instance number of the class (0 if first/unique instance, >0 otherwise)
  * @retval The Class ID (this function does not set the pdev->classId field.
  */
uint32_t  USBD_CMPSIT_GetClassID(USBD_HandleTypeDef *pdev, USBD_CompositeClassTypeDef Class, uint32_t Instance)
{
  uint32_t idx;
  uint32_t inst = 0U;

  /* Unroll all already activated classes */
  for (idx = 0U; idx < pdev->NumClasses; idx++)
  {
    /* Check if the class correspond to the requested type and if it is active */
    if (((USBD_CompositeClassTypeDef)(pdev->tclasslist[idx].ClassType) == Class) &&
        ((pdev->tclasslist[idx].Active) == 1U))
    {
      if (inst == Instance)
      {
        /* Return the class ID value */
        return (idx);
      }
      else
      {
        /* Increment instance index and look for next instance */
        inst++;
      }
    }
  }

  /* No class found, return 0xFF */
  return 0xFFU;
}

/**
  * @brief  USBD_CMPST_ClearConfDesc
  *         Reset the configuration descriptor
  * @param  pdev: device instance (reserved for future use)
  * @retval Status.
  */
uint8_t USBD_CMPST_ClearConfDesc(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  /* Reset the configuration descriptor pointer to default value and its size to zero */
  pCmpstFSConfDesc = USBD_CMPSIT_FSCfgDesc;
  CurrFSConfDescSz = 0U;

#ifdef USE_USB_HS
  pCmpstHSConfDesc = USBD_CMPSIT_HSCfgDesc;
  CurrHSConfDescSz = 0U;
#endif /* USE_USB_HS */

  /* All done, can't fail */
  return (uint8_t)USBD_OK;
}

#endif /* USE_USBD_COMPOSITE */

/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */
