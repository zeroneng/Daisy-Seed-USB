#define DSY_RAM_D2 __attribute__((section(".heap")))
#include "daisy_seed.h"
#include "usbd_audio_if.h"

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_audio.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

DaisySeed hw;

static void InitUSBAudio(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_AUDIO);
    USBD_AUDIO_RegisterInterface(&hUsbDeviceHS, &USBD_AUDIO_fops);
    USBD_Start(&hUsbDeviceHS);
}

int main(void)
{
    hw.Init();
    InitUSBAudio();

    for(;;) {}
}
