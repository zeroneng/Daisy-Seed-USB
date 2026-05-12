#include "daisy_seed.h"
#include "per/sdmmc.h"

using namespace daisy;

extern "C" {
#include "stm32h7xx_hal.h"
extern SD_HandleTypeDef hsd1;

void STORAGE_SD_UserInit(int use_sd_1bit)
{
    static SdmmcHandler sdcard;
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd_cfg.width = use_sd_1bit ? SdmmcHandler::BusWidth::BITS_1
                               : SdmmcHandler::BusWidth::BITS_4;
    sd_cfg.speed = SdmmcHandler::Speed::STANDARD;
    sdcard.Init(sd_cfg);
}

static int InitCardTryWidth(int use_sd_1bit)
{
    HAL_StatusTypeDef status = HAL_SD_Init(&hsd1);
    if(status != HAL_OK)
        return -1;

    if(!use_sd_1bit)
    {
        if(HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK)
            return -1;
    }

    return 0;
}

int STORAGE_SD_RefreshInfo(uint32_t* block_count, int allow_fallback_to_1bit)
{
    HAL_SD_CardInfoTypeDef card_info;

    if(InitCardTryWidth(0) != 0)
    {
        if(!allow_fallback_to_1bit || InitCardTryWidth(1) != 0)
            return -1;
    }

    if(HAL_SD_GetCardInfo(&hsd1, &card_info) != HAL_OK)
        return -1;

    if(block_count)
        *block_count = card_info.LogBlockNbr;
    return card_info.LogBlockNbr > 0 ? 0 : -1;
}

int STORAGE_SD_ReadBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    if(HAL_SD_ReadBlocks(&hsd1, buf, blk_addr, blk_len, HAL_MAX_DELAY) != HAL_OK)
        return -1;
    while(HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {}
    return 0;
}

int STORAGE_SD_WriteBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    if(HAL_SD_WriteBlocks(&hsd1, buf, blk_addr, blk_len, HAL_MAX_DELAY) != HAL_OK)
        return -1;
    while(HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {}
    return 0;
}
}
