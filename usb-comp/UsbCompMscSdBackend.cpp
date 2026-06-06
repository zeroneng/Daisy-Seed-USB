#include "daisy_seed.h"
#include "per/sdmmc.h"
#include "util/bsp_sd_diskio.h"

using namespace daisy;

extern "C" {

#define STORAGE_SD_SECTOR_SIZE 512U
#define STORAGE_SD_TRANSFER_TIMEOUT_MS 1000U

static int SD_WaitReady(void)
{
    const uint32_t start = System::GetNow();
    do
    {
        if(BSP_SD_GetCardState() == SD_TRANSFER_OK)
            return 0;
    } while((System::GetNow() - start) < STORAGE_SD_TRANSFER_TIMEOUT_MS);

    return -1;
}

int STORAGE_SD_Init(uint32_t* block_count)
{
    static SdmmcHandler sdcard;

    if(block_count == nullptr)
        return -1;

    SdmmcHandler::Config sd_cfg;
    sd_cfg.width = SdmmcHandler::BusWidth::BITS_4;
    sd_cfg.speed = SdmmcHandler::Speed::MEDIUM_SLOW;
    sd_cfg.clock_powersave = false;

    if(sdcard.Init(sd_cfg) != SdmmcHandler::Result::OK)
        return -1;

    if(BSP_SD_Init() != MSD_OK)
        return -1;

    BSP_SD_CardInfo card_info;
    BSP_SD_GetCardInfo(&card_info);

    *block_count = card_info.LogBlockNbr;
    return *block_count > 0 ? 0 : -1;
}

int STORAGE_SD_ReadBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    if(buf == nullptr || blk_len == 0)
        return -1;

    for(uint16_t i = 0; i < blk_len; ++i)
    {
        if(BSP_SD_ReadBlocks(
               (uint32_t*)(buf + ((uint32_t)i * STORAGE_SD_SECTOR_SIZE)),
               blk_addr + i,
               1,
               STORAGE_SD_TRANSFER_TIMEOUT_MS)
           != MSD_OK)
            return -1;
        if(SD_WaitReady() != 0)
            return -1;
    }
    return 0;
}

int STORAGE_SD_WriteBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    if(buf == nullptr || blk_len == 0)
        return -1;

    for(uint16_t i = 0; i < blk_len; ++i)
    {
        if(BSP_SD_WriteBlocks(
               (uint32_t*)(buf + ((uint32_t)i * STORAGE_SD_SECTOR_SIZE)),
               blk_addr + i,
               1,
               STORAGE_SD_TRANSFER_TIMEOUT_MS)
           != MSD_OK)
            return -1;
        if(SD_WaitReady() != 0)
            return -1;
    }
    return 0;
}
}
