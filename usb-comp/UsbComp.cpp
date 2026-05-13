#include "daisy_seed.h"
#include <cstring>

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_def.h"
#include "usbd_cdc.h"
#include "usbd_hid.h"
#include "usb_comp_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
uint32_t USBD_CMPSIT_SetClassID(USBD_HandleTypeDef *pdev,
                                USBD_CompositeClassTypeDef Class,
                                uint32_t Instance);
}

DaisySeed hw;
static uint8_t hid_report[4] = {0, 0, 0, 0};
static uint32_t hid_class_id = 0;
static uint32_t cdc_class_id = 0;

static void InitUSBComposite(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);

    hid_class_id = USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_HID, 0);
    USBD_RegisterClass(&hUsbDeviceHS, USBD_HID_CLASS);

    cdc_class_id = USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_CDC, 0);
    USBD_RegisterClass(&hUsbDeviceHS, USBD_CDC_CLASS);
    USB_COMP_CDC_SetClassId((uint8_t)cdc_class_id);
    USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USB_COMP_CDC_Interface_fops_HS);

    USBD_Start(&hUsbDeviceHS);
}

static void SendString(const char* s)
{
    if(!s)
        return;
    USBD_CDC_SetTxBuffer(&hUsbDeviceHS, (uint8_t*)s, (uint16_t)strlen(s), (uint8_t)cdc_class_id);
    USBD_CDC_TransmitPacket(&hUsbDeviceHS, (uint8_t)cdc_class_id);
}

static void SendHidPulse()
{
    USBD_HID_SendReport(&hUsbDeviceHS, hid_report, sizeof(hid_report), (uint8_t)hid_class_id);
}

int main(void)
{
    hw.Init();
    InitUSBComposite();

    bool led = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        SendString(led ? "COMP LED ON\r\n" : "COMP LED OFF\r\n");
        hid_report[0] = 0;
        hid_report[1] = 0;
        hid_report[2] = led ? 1 : 0;
        hid_report[3] = 0;
        SendHidPulse();
        System::Delay(250);
    }
}
