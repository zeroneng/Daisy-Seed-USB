#include "usb_comp_cdc_if.h"
#include <string.h>

#define APP_RX_DATA_SIZE 2048
#define APP_TX_DATA_SIZE 2048

uint8_t UserRxBufferHS[APP_RX_DATA_SIZE];
uint8_t UserTxBufferHS[APP_TX_DATA_SIZE];

extern USBD_HandleTypeDef hUsbDeviceHS;

static uint8_t cdc_class_id_hs = 0;
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

void USB_COMP_CDC_SetClassId(uint8_t class_id)
{
    cdc_class_id_hs = class_id;
}

void CDC_Set_Rx_Callback_HS(CDC_ReceiveCallback cb)
{
    rx_callback_hs = cb;
}

static int8_t CDC_Init_HS(void)
{
    USBD_CDC_SetTxBuffer(&hUsbDeviceHS, UserTxBufferHS, 0, cdc_class_id_hs);
    USBD_CDC_SetRxBuffer(&hUsbDeviceHS, UserRxBufferHS);
    if(!rx_callback_hs)
        rx_callback_hs = dummy_rx_callback;
    return (USBD_OK);
}

static int8_t CDC_DeInit_HS(void)
{
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
        default:
            break;
    }
    return (USBD_OK);
}

static int8_t CDC_Receive_HS_Int(uint8_t* pbuf, uint32_t* Len)
{
    if(rx_callback_hs)
        rx_callback_hs(pbuf, Len);
    USBD_CDC_SetRxBuffer(&hUsbDeviceHS, pbuf);
    USBD_CDC_ReceivePacket(&hUsbDeviceHS);
    return (USBD_OK);
}

uint8_t CDC_Transmit_HS(uint8_t* Buf, uint16_t Len)
{
    USBD_CDC_HandleTypeDef* hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceHS.pClassDataCmsit[cdc_class_id_hs];
    if(hcdc == NULL || hcdc->TxState != 0U)
        return USBD_BUSY;
    USBD_CDC_SetTxBuffer(&hUsbDeviceHS, Buf, Len, cdc_class_id_hs);
    return USBD_CDC_TransmitPacket(&hUsbDeviceHS, cdc_class_id_hs);
}
