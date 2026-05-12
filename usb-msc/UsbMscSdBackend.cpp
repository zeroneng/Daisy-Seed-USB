#include "daisy_seed.h"
#include "per/sdmmc.h"

using namespace daisy;

extern "C" {
#include "stm32h7xx_hal.h"
extern SD_HandleTypeDef hsd1;

int STORAGE_SD_ReadBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    for(uint16_t i = 0; i < blk_len; ++i)
    {
        if(HAL_SD_ReadBlocks(&hsd1,
                             buf + ((uint32_t)i * 512U),
                             blk_addr + i,
                             1,
                             HAL_MAX_DELAY)
           != HAL_OK)
            return -1;
        while(HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {}
    }
    return 0;
}

int STORAGE_SD_WriteBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    for(uint16_t i = 0; i < blk_len; ++i)
    {
        if(HAL_SD_WriteBlocks(&hsd1,
                              buf + ((uint32_t)i * 512U),
                              blk_addr + i,
                              1,
                              HAL_MAX_DELAY)
           != HAL_OK)
            return -1;
        while(HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {}
    }
    return 0;
}
}
