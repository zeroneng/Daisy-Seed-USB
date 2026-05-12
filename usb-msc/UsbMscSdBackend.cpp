#include "daisy_seed.h"
#include "per/sdmmc.h"

using namespace daisy;

extern "C" {
#include "stm32h7xx_hal.h"
extern SD_HandleTypeDef hsd1;

#define STORAGE_SD_SECTOR_SIZE 512U
#define STORAGE_SD_STAGE_SECTORS 16U
#define STORAGE_SD_STAGE_BYTES (STORAGE_SD_STAGE_SECTORS * STORAGE_SD_SECTOR_SIZE)

static uint8_t g_sd_stage_buf[STORAGE_SD_STAGE_BYTES] __attribute__((aligned(32)));

static int SD_WaitReady(void)
{
    while(HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {}
    return 0;
}

static int SD_ReadSingle(uint8_t* buf, uint32_t blk_addr)
{
    if(HAL_SD_ReadBlocks(&hsd1, buf, blk_addr, 1, HAL_MAX_DELAY) != HAL_OK)
        return -1;
    return SD_WaitReady();
}

static int SD_WriteMulti(uint8_t* buf, uint32_t blk_addr, uint32_t blk_count)
{
    if(blk_count == 0)
        return 0;
    if(HAL_SD_WriteBlocks(&hsd1, buf, blk_addr, blk_count, HAL_MAX_DELAY) != HAL_OK)
        return -1;
    return SD_WaitReady();
}

int STORAGE_SD_ReadBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    for(uint16_t i = 0; i < blk_len; ++i)
    {
        if(SD_ReadSingle(buf + ((uint32_t)i * STORAGE_SD_SECTOR_SIZE), blk_addr + i) != 0)
            return -1;
    }
    return 0;
}

int STORAGE_SD_WriteBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    uint32_t remaining = blk_len;
    uint32_t src_index = 0;

    while(remaining > 0)
    {
        uint32_t chunk = remaining < STORAGE_SD_STAGE_SECTORS ? remaining : STORAGE_SD_STAGE_SECTORS;

        for(uint32_t i = 0; i < chunk; ++i)
        {
            uint32_t dst_off = i * STORAGE_SD_SECTOR_SIZE;
            uint32_t src_off = (src_index + i) * STORAGE_SD_SECTOR_SIZE;
            for(uint32_t j = 0; j < STORAGE_SD_SECTOR_SIZE; ++j)
                g_sd_stage_buf[dst_off + j] = buf[src_off + j];
        }

        if(SD_WriteMulti(g_sd_stage_buf, blk_addr + src_index, chunk) != 0)
            return -1;

        src_index += chunk;
        remaining -= chunk;
    }

    return 0;
}
}
