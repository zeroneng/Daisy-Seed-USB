#include "daisy_seed.h"

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_msc_storage.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
extern volatile uint32_t g_storage_diag_state;
extern volatile uint32_t g_storage_diag_counter;
}

DaisySeed hw;

static void InitUSBMSC(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_MSC);
    USBD_MSC_RegisterStorage(&hUsbDeviceHS, &USBD_MSC_Template_fops);
    USBD_Start(&hUsbDeviceHS);
}

int main(void)
{
    hw.Init();
    STORAGE_UserInit();
    InitUSBMSC();

    for(;;)
    {
        uint32_t phase = (System::GetNow() / 125) & 0x7;
        bool led = false;

        switch(g_storage_diag_state)
        {
            case 0:
                led = (phase == 0);
                break;
            case 1:
                led = (phase == 0 || phase == 2);
                break;
            case 2:
                led = (phase == 0 || phase == 2 || phase == 4);
                break;
            case 3:
                led = (phase == 0 || phase == 2 || phase == 4 || phase == 6);
                break;
            case 4:
                led = true;
                break;
            default:
                led = ((System::GetNow() & 127) < 64);
                break;
        }

        if(g_storage_diag_counter != 0 && (System::GetNow() & 511) < 64)
            led = !led;

        hw.SetLed(led);
        System::Delay(20);
    }
}
