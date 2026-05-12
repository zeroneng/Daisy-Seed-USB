#include "daisy_seed.h"
#include "per/gpio.h"
#include "per/sdmmc.h"

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_msc_storage.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
extern SD_HandleTypeDef hsd1;
}

DaisySeed hw;
GPIO sd_cd;
SdmmcHandler sdcard;

static void InitUSBMSC(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_MSC);
    USBD_MSC_RegisterStorage(&hUsbDeviceHS, &USBD_MSC_Template_fops);
    USBD_Start(&hUsbDeviceHS);
}

static void BlinkFatal(int pulses)
{
    while(1)
    {
        for(int i = 0; i < pulses; ++i)
        {
            hw.SetLed(true);
            System::Delay(120);
            hw.SetLed(false);
            System::Delay(120);
        }
        System::Delay(5000);
    }
}

int main(void)
{
    hw.Init();
    STORAGE_UserInit();

    GPIO::Config cd_cfg;
    cd_cfg.pin = seed::D0;
    cd_cfg.mode = GPIO::Mode::INPUT;
    cd_cfg.pull = GPIO::Pull::NOPULL;
    sd_cd.Init(cd_cfg);

    bool cardinserted = !sd_cd.Read();
    if(!cardinserted)
    {
        g_msc_diag_state = 1;
        BlinkFatal(1);
    }

    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd_cfg.width = SdmmcHandler::BusWidth::BITS_4;
    sd_cfg.speed = SdmmcHandler::Speed::STANDARD;

    if(sdcard.Init(sd_cfg) != SdmmcHandler::Result::OK)
    {
        g_msc_diag_state = 2;
        BlinkFatal(2);
    }

    bool using_1bit = false;
    if(HAL_SD_Init(&hsd1) != HAL_OK)
    {
        sd_cfg.width = SdmmcHandler::BusWidth::BITS_1;
        sdcard.Init(sd_cfg);
        using_1bit = true;
        if(HAL_SD_Init(&hsd1) != HAL_OK)
        {
            g_msc_diag_state = 3;
            BlinkFatal(3);
        }
    }
    else
    {
        if(HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK)
        {
            g_msc_diag_state = 4;
            BlinkFatal(4);
        }
    }

    HAL_SD_CardInfoTypeDef card_info;
    if(HAL_SD_GetCardInfo(&hsd1, &card_info) != HAL_OK)
    {
        g_msc_diag_state = 5;
        BlinkFatal(5);
    }

    g_msc_sd_block_count = card_info.LogBlockNbr;
    g_msc_sd_ready = g_msc_sd_block_count > 0 ? 1 : 0;
    g_msc_diag_state = using_1bit ? 8 : 9;

    InitUSBMSC();

    for(;;)
    {
        bool led = false;
        uint32_t phase = (System::GetNow() / 125) & 0x7;

        switch(g_msc_diag_state)
        {
            case 6:
                led = true;
                break;
            case 7:
                led = (phase < 4);
                break;
            case 8:
                led = ((System::GetNow() / 2000) & 1) != 0;
                break;
            case 9:
                led = ((System::GetNow() / 1000) & 1) != 0;
                break;
            default:
                led = (phase == 0);
                break;
        }

        if(g_msc_diag_counter != 0 && (System::GetNow() & 511) < 64)
            led = !led;

        hw.SetLed(led);
        System::Delay(20);
    }
}
