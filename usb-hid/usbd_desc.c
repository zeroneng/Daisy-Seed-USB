/**
  ******************************************************************************
  * @file    usbd_desc.c
  * @brief   USB Device descriptors for Daisy Seed HID device
  ******************************************************************************
  */

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"

#define USBD_VID                     1155
#define USBD_LANGID_STRING           1033
#define USBD_MANUFACTURER_STRING     "ZERONE"

#define USBD_PID_FS                  0x5750
#define USBD_PRODUCT_STRING_FS       "RHYTHM HID"
#define USBD_CONFIGURATION_STRING_FS "HID Config"
#define USBD_INTERFACE_STRING_FS     "HID Interface"

#define USBD_PID_HS                  0x5751
#define USBD_PRODUCT_STRING_HS       "RHYTHM HID"
#define USBD_CONFIGURATION_STRING_HS "HID Config"
#define USBD_INTERFACE_STRING_HS     "HID Interface"

#define DEVICE_ID1 (UID_BASE)
#define DEVICE_ID2 (UID_BASE + 0x4)
#define DEVICE_ID3 (UID_BASE + 0x8)
#define USB_SIZ_STRING_SERIAL 0x1A

uint8_t *USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);

uint8_t *USBD_HS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_HS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_HS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_HS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_HS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_HS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_HS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);

USBD_DescriptorsTypeDef FS_Desc = {
    USBD_FS_DeviceDescriptor,
    USBD_FS_LangIDStrDescriptor,
    USBD_FS_ManufacturerStrDescriptor,
    USBD_FS_ProductStrDescriptor,
    USBD_FS_SerialStrDescriptor,
    USBD_FS_ConfigStrDescriptor,
    USBD_FS_InterfaceStrDescriptor,
};

USBD_DescriptorsTypeDef HS_Desc = {
    USBD_HS_DeviceDescriptor,
    USBD_HS_LangIDStrDescriptor,
    USBD_HS_ManufacturerStrDescriptor,
    USBD_HS_ProductStrDescriptor,
    USBD_HS_SerialStrDescriptor,
    USBD_HS_ConfigStrDescriptor,
    USBD_HS_InterfaceStrDescriptor,
};

#if defined ( __ICCARM__ )
#pragma data_alignment=4
#endif
__ALIGN_BEGIN uint8_t USBD_FS_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12, USB_DESC_TYPE_DEVICE,
    0x00, 0x02,
    0x00, 0x00, 0x00,
    USB_MAX_EP0_SIZE,
    LOBYTE(USBD_VID), HIBYTE(USBD_VID),
    LOBYTE(USBD_PID_FS), HIBYTE(USBD_PID_FS),
    0x00, 0x02,
    USBD_IDX_MFC_STR, USBD_IDX_PRODUCT_STR, USBD_IDX_SERIAL_STR,
    USBD_MAX_NUM_CONFIGURATION
};

__ALIGN_BEGIN uint8_t USBD_HS_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12, USB_DESC_TYPE_DEVICE,
    0x00, 0x02,
    0x00, 0x00, 0x00,
    USB_MAX_EP0_SIZE,
    LOBYTE(USBD_VID), HIBYTE(USBD_VID),
    LOBYTE(USBD_PID_HS), HIBYTE(USBD_PID_HS),
    0x00, 0x02,
    USBD_IDX_MFC_STR, USBD_IDX_PRODUCT_STR, USBD_IDX_SERIAL_STR,
    USBD_MAX_NUM_CONFIGURATION
};

__ALIGN_BEGIN uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING)
};

__ALIGN_BEGIN uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL,
    USB_DESC_TYPE_STRING,
};

__ALIGN_BEGIN uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    for(uint8_t idx = 0; idx < len; idx++)
    {
        uint8_t nib = (value >> 28) & 0x0F;
        pbuf[2 * idx] = (nib < 10) ? ('0' + nib) : ('A' + nib - 10);
        pbuf[2 * idx + 1] = 0;
        value <<= 4;
    }
}

static void Get_SerialNum(void)
{
    uint32_t deviceserial0 = *(uint32_t *)DEVICE_ID1;
    uint32_t deviceserial1 = *(uint32_t *)DEVICE_ID2;
    uint32_t deviceserial2 = *(uint32_t *)DEVICE_ID3;
    deviceserial0 += deviceserial2;
    if(deviceserial0 != 0)
    {
        IntToUnicode(deviceserial0, &USBD_StringSerial[2], 8);
        IntToUnicode(deviceserial1, &USBD_StringSerial[18], 4);
    }
}

uint8_t *USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(USBD_FS_DeviceDesc);
    return USBD_FS_DeviceDesc;
}

uint8_t *USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(USBD_LangIDDesc);
    return USBD_LangIDDesc;
}

uint8_t *USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

uint8_t *USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_PRODUCT_STRING_FS, USBD_StrDesc, length);
    return USBD_StrDesc;
}

uint8_t *USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = USB_SIZ_STRING_SERIAL;
    Get_SerialNum();
    return USBD_StringSerial;
}

uint8_t *USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING_FS, USBD_StrDesc, length);
    return USBD_StrDesc;
}

uint8_t *USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_INTERFACE_STRING_FS, USBD_StrDesc, length);
    return USBD_StrDesc;
}

uint8_t *USBD_HS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(USBD_HS_DeviceDesc);
    return USBD_HS_DeviceDesc;
}
uint8_t *USBD_HS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ return USBD_FS_LangIDStrDescriptor(speed, length); }
uint8_t *USBD_HS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ return USBD_FS_ManufacturerStrDescriptor(speed, length); }
uint8_t *USBD_HS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_PRODUCT_STRING_HS, USBD_StrDesc, length);
    return USBD_StrDesc;
}
uint8_t *USBD_HS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{ return USBD_FS_SerialStrDescriptor(speed, length); }
uint8_t *USBD_HS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING_HS, USBD_StrDesc, length);
    return USBD_StrDesc;
}
uint8_t *USBD_HS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_INTERFACE_STRING_HS, USBD_StrDesc, length);
    return USBD_StrDesc;
}
