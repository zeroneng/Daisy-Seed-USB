#include "usb_comp_cdc_if.h"
#include <string.h>

#ifndef USB_COMP_ENABLE_MSC
#define USB_COMP_ENABLE_MSC 0
#endif

uint8_t UserRxBufferHS[APP_RX_DATA_SIZE];
uint8_t UserTxBufferHS[APP_TX_DATA_SIZE];
uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

extern USBD_HandleTypeDef hUsbDeviceHS;

static uint8_t cdc_class_id_hs = 0;
static volatile uint8_t cdc_connected_hs = 0;
static volatile uint8_t msc_command_hs = 0;
static CDC_ReceiveCallback rx_callback_hs = 0;
static void dummy_rx_callback(uint8_t* buf, uint32_t* len)
{
    (void)buf;
    (void)len;
}

static int8_t CDC_Init_HS(void);
static int8_t CDC_DeInit_HS(void);
static int8_t CDC_Control_HS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_HS_Int(uint8_t* pbuf, uint32_t* Len);

USBD_CDC_ItfTypeDef USB_COMP_CDC_Interface_fops_HS = {
    CDC_Init_HS,
    CDC_DeInit_HS,
    CDC_Control_HS,
    CDC_Receive_HS_Int,
    NULL,
};

USBD_CDC_ItfTypeDef USBD_Interface_fops_HS = {
    CDC_Init_HS,
    CDC_DeInit_HS,
    CDC_Control_HS,
    CDC_Receive_HS_Int,
    NULL,
};

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {
    CDC_Init_HS,
    CDC_DeInit_HS,
    CDC_Control_HS,
    CDC_Receive_HS_Int,
    NULL,
};

void USB_COMP_CDC_SetClassId(uint8_t class_id)
{
    cdc_class_id_hs = class_id;
}

uint8_t USB_COMP_CDC_IsConnected(void)
{
    return cdc_connected_hs;
}

uint8_t USB_COMP_CDC_TakeMscCommand(void)
{
    uint8_t command = msc_command_hs;
    msc_command_hs = 0U;
    return command;
}

void CDC_Set_Rx_Callback_HS(CDC_ReceiveCallback cb)
{
    rx_callback_hs = cb;
}

static int8_t CDC_Init_HS(void)
{
    cdc_connected_hs = 0;
    USBD_CDC_SetTxBuffer(&hUsbDeviceHS, UserTxBufferHS, 0, cdc_class_id_hs);
    USBD_CDC_SetRxBuffer(&hUsbDeviceHS, UserRxBufferHS);
    if(!rx_callback_hs)
        rx_callback_hs = dummy_rx_callback;
    return (USBD_OK);
}

static int8_t CDC_DeInit_HS(void)
{
    cdc_connected_hs = 0;
    return (USBD_OK);
}

static uint8_t line_coding_hs[7] = {0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08};

static int8_t CDC_Control_HS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
    (void)length;
    switch(cmd)
    {
        case CDC_SET_LINE_CODING:
            memcpy(line_coding_hs, pbuf, sizeof(line_coding_hs));
            break;
        case CDC_GET_LINE_CODING:
            memcpy(pbuf, line_coding_hs, sizeof(line_coding_hs));
            break;
        case CDC_SET_CONTROL_LINE_STATE:
            break;
        default:
            break;
    }
    return (USBD_OK);
}

static int8_t CDC_Receive_HS_Int(uint8_t* pbuf, uint32_t* Len)
{
    cdc_connected_hs = 1U;
#if USB_COMP_ENABLE_MSC
    if(pbuf && Len && *Len > 0U)
    {
        /* Queue only a tiny command marker from the USB RX callback.  MSC
           enable may initialize SD media, so UsbComp.cpp consumes this marker
           later from the main loop. */
        for(uint32_t i = 0; i < *Len; ++i)
        {
            if(pbuf[i] == 'M' || pbuf[i] == 'm' || pbuf[i] == 'S')
                msc_command_hs = pbuf[i];
        }
    }
#endif
#if USB_COMP_TEST_CDC
    if(Len && *Len > 0U)
    {
        static uint8_t rx_msg[] = "COMP CDC RX\r\n";
        (void)CDC_Transmit_HS(rx_msg, sizeof(rx_msg) - 1U);
    }
#endif
    if(rx_callback_hs)
        rx_callback_hs(pbuf, Len);
    USBD_CDC_SetRxBuffer(&hUsbDeviceHS, pbuf);
    USBD_CDC_ReceivePacket(&hUsbDeviceHS);
    return (USBD_OK);
}

uint8_t CDC_Transmit_HS(uint8_t* Buf, uint16_t Len)
{
    USBD_CDC_HandleTypeDef* hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceHS.pClassDataCmsit[cdc_class_id_hs];
    if(cdc_connected_hs == 0U || hcdc == NULL || hcdc->TxState != 0U)
        return USBD_BUSY;
    USBD_CDC_SetTxBuffer(&hUsbDeviceHS, Buf, Len, cdc_class_id_hs);
    return USBD_CDC_TransmitPacket(&hUsbDeviceHS, cdc_class_id_hs);
}

void CDC_Set_Rx_Callback_FS(CDC_ReceiveCallback cb)
{
    CDC_Set_Rx_Callback_HS(cb);
}

uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
    return CDC_Transmit_HS(Buf, Len);
}
