#include "usbd_msc_storage.h"

#define STORAGE_LUN_NBR 1U
#define STORAGE_BLK_SIZ 512U

volatile uint32_t g_msc_diag_state = 0;
volatile uint32_t g_msc_diag_counter = 0;
volatile uint32_t g_msc_sd_ready = 0;
volatile uint32_t g_msc_sd_block_count = 0;

extern int STORAGE_SD_ReadBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
extern int STORAGE_SD_WriteBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);

static int8_t STORAGE_Init(uint8_t lun);
static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t* block_num, uint16_t* block_size);
static int8_t STORAGE_IsReady(uint8_t lun);
static int8_t STORAGE_IsWriteProtected(uint8_t lun);
static int8_t STORAGE_Read(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun(void);

static int8_t STORAGE_Inquirydata[] = {
    0x00,
    0x80,
    0x02,
    0x02,
    (STANDARD_INQUIRY_DATA_LEN - 5),
    0x00,
    0x00,
    0x00,
    'Z','E','R','O','N','E',' ',' ',
    'R','H','Y','T','H','M',' ','S',
    'D',' ','S','T','O','R','E',' ',
    '0','.','1','0'
};

USBD_StorageTypeDef USBD_MSC_Template_fops = {
    STORAGE_Init,
    STORAGE_GetCapacity,
    STORAGE_IsReady,
    STORAGE_IsWriteProtected,
    STORAGE_Read,
    STORAGE_Write,
    STORAGE_GetMaxLun,
    STORAGE_Inquirydata,
};

void STORAGE_UserInit(void)
{
    g_msc_diag_counter = 0;
}

static int8_t STORAGE_Init(uint8_t lun)
{
    UNUSED(lun);
    if(!g_msc_sd_ready || g_msc_sd_block_count == 0)
    {
        g_msc_diag_state = 2;
        return -1;
    }
    g_msc_diag_state = 3;
    return 0;
}

static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t* block_num, uint16_t* block_size)
{
    UNUSED(lun);
    if(!g_msc_sd_ready || g_msc_sd_block_count == 0)
    {
        g_msc_diag_state = 2;
        return -1;
    }
    *block_num = g_msc_sd_block_count;
    *block_size = STORAGE_BLK_SIZ;
    g_msc_diag_state = 3;
    return 0;
}

static int8_t STORAGE_IsReady(uint8_t lun)
{
    UNUSED(lun);
    if(!g_msc_sd_ready || g_msc_sd_block_count == 0)
    {
        g_msc_diag_state = 2;
        return -1;
    }
    return 0;
}

static int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
    UNUSED(lun);
    return 0;
}

static int8_t STORAGE_Read(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    UNUSED(lun);
    g_msc_diag_counter++;
    if(STORAGE_IsReady(lun) != 0)
        return -1;
    if((uint64_t)blk_addr + blk_len > g_msc_sd_block_count)
    {
        g_msc_diag_state = 4;
        return -1;
    }
    if(STORAGE_SD_ReadBlocks(buf, blk_addr, blk_len) != 0)
    {
        g_msc_diag_state = 5;
        return -1;
    }
    g_msc_diag_state = 6;
    return 0;
}

static int8_t STORAGE_Write(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    UNUSED(lun);
    g_msc_diag_counter++;
    if(STORAGE_IsReady(lun) != 0)
        return -1;
    if((uint64_t)blk_addr + blk_len > g_msc_sd_block_count)
    {
        g_msc_diag_state = 4;
        return -1;
    }
    if(STORAGE_SD_WriteBlocks(buf, blk_addr, blk_len) != 0)
    {
        g_msc_diag_state = 5;
        return -1;
    }
    g_msc_diag_state = 7;
    return 0;
}

static int8_t STORAGE_GetMaxLun(void)
{
    return STORAGE_LUN_NBR - 1;
}
