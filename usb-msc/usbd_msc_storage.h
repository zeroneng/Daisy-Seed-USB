/**
  ******************************************************************************
  * @file    usbd_msc_storage.h
  * @brief   Header file for the usbd_msc_storage.c file
  ******************************************************************************
  */

#ifndef __USBD_MSC_STORAGE_H
#define __USBD_MSC_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_msc.h"

extern USBD_StorageTypeDef USBD_MSC_Template_fops;
void STORAGE_UserInit(void);

#ifdef __cplusplus
}
#endif

#endif
