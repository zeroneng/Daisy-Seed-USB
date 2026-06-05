#include "daisy_seed.h"

#include <cstring>

using namespace daisy;

#ifndef USB_COMP_ENABLE_CDC
#define USB_COMP_ENABLE_CDC 1
#endif

#ifndef USB_COMP_ENABLE_HID
#define USB_COMP_ENABLE_HID 1
#endif

#ifndef USB_COMP_TEST_CDC
#define USB_COMP_TEST_CDC 1
#endif

#ifndef USB_COMP_TEST_HID
#define USB_COMP_TEST_HID 1
#endif

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_def.h"
#include "usb_comp_cdc_if.h"

#if USB_COMP_ENABLE_CDC
#include "usbd_cdc.h"
#endif

#if USB_COMP_ENABLE_HID
#include "usbd_hid.h"
#endif

extern USBD_HandleTypeDef hUsbDeviceHS;
extern USBD_ClassTypeDef USBD_CMPSIT;
uint32_t USBD_CMPSIT_SetClassID(USBD_HandleTypeDef *pdev,
                                USBD_CompositeClassTypeDef Class,
                                uint32_t Instance);
uint32_t USBD_CMPSIT_GetClassID(USBD_HandleTypeDef *pdev,
                                USBD_CompositeClassTypeDef Class,
                                uint32_t Instance);
}

namespace {
DaisySeed hw;

constexpr uint8_t kNoClass = 0xFFU;
constexpr uint8_t kNkroReportBytes = 33;
constexpr uint8_t kNkroBitmapOffset = 1;
constexpr uint8_t kHidKeyA = 0x04;

uint8_t hid_report[kNkroReportBytes] = {0};
uint8_t hid_class_id = kNoClass;
uint8_t cdc_class_id = kNoClass;

#if USB_COMP_ENABLE_HID
uint8_t hid_ep_addr[] = {0x81U};
#endif

#if USB_COMP_ENABLE_CDC
uint8_t cdc_ep_addr[] = {0x82U, 0x02U, 0x83U};
#endif

void SetKeyState(uint8_t keycode, bool pressed)
{
    if(keycode > 0xE7)
        return;

    if(keycode >= 0xE0 && keycode <= 0xE7)
    {
        const uint8_t bit_mask = static_cast<uint8_t>(1u << (keycode - 0xE0));
        if(pressed)
            hid_report[0] |= bit_mask;
        else
            hid_report[0] &= static_cast<uint8_t>(~bit_mask);
        return;
    }

    const uint8_t byte_index = static_cast<uint8_t>(keycode >> 3);
    if(byte_index >= (kNkroReportBytes - kNkroBitmapOffset))
        return;

    const uint8_t bit_mask = static_cast<uint8_t>(1u << (keycode & 0x07u));
    uint8_t& byte = hid_report[kNkroBitmapOffset + byte_index];
    if(pressed)
        byte |= bit_mask;
    else
        byte &= static_cast<uint8_t>(~bit_mask);
}

void InitUSBComposite()
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);

#if USB_COMP_ENABLE_HID
    USBD_RegisterClassComposite(&hUsbDeviceHS, USBD_HID_CLASS, CLASS_TYPE_HID, hid_ep_addr);
    hid_class_id = static_cast<uint8_t>(USBD_CMPSIT_GetClassID(&hUsbDeviceHS, CLASS_TYPE_HID, 0));
#endif

#if USB_COMP_ENABLE_CDC
    USBD_RegisterClassComposite(&hUsbDeviceHS, USBD_CDC_CLASS, CLASS_TYPE_CDC, cdc_ep_addr);
    cdc_class_id = static_cast<uint8_t>(USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_CDC, 0));
    USB_COMP_CDC_SetClassId(cdc_class_id);
    USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USB_COMP_CDC_Interface_fops_HS);
#endif

    USBD_Start(&hUsbDeviceHS);
}

bool SendCdcString(const char* s)
{
#if USB_COMP_ENABLE_CDC
    if(!s || cdc_class_id == kNoClass || !USB_COMP_CDC_IsConnected())
        return false;

    return CDC_Transmit_HS((uint8_t*)s, (uint16_t)strlen(s)) == USBD_OK;
#else
    (void)s;
    return false;
#endif
}

void SendHidReport()
{
#if USB_COMP_ENABLE_HID
    if(hid_class_id == kNoClass)
        return;

    USBD_HID_SendReport(&hUsbDeviceHS, hid_report, sizeof(hid_report), hid_class_id);
#endif
}

void RunCdcTest(bool led)
{
#if USB_COMP_ENABLE_CDC && USB_COMP_TEST_CDC
    SendCdcString(led ? "COMP CDC LED ON\r\n" : "COMP CDC LED OFF\r\n");
#else
    (void)led;
#endif
}

void RunHidTest()
{
#if USB_COMP_ENABLE_HID && USB_COMP_TEST_HID
    std::memset(hid_report, 0, sizeof(hid_report));
    SetKeyState(kHidKeyA, true);
    SendHidReport();
    System::Delay(40);
    SetKeyState(kHidKeyA, false);
    SendHidReport();
#endif
}
} // namespace

int main(void)
{
    hw.Init();
    InitUSBComposite();

    System::Delay(1500);

    SendHidReport();

    bool led = false;
    bool cdc_ready_sent = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        if(!cdc_ready_sent)
            cdc_ready_sent = SendCdcString("COMP CDC NKRO ready\r\n");
        RunCdcTest(led);
        RunHidTest();
        System::Delay(1000);
    }
}
