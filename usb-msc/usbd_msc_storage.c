#include "usbd_msc_storage.h"

#define USE_SD_CARD 0
#define USE_SD_1BIT 0
#define ALLOW_SD_FALLBACK_TO_1BIT 1

#define STORAGE_LUN_NBR 1U
#define STORAGE_BLK_SIZ 512U

volatile uint32_t g_storage_diag_state = 0;
volatile uint32_t g_storage_diag_counter = 0;

#if USE_SD_CARD
#include "stm32h7xx_hal.h"
static uint8_t sd_ready = 0;
static uint32_t sd_block_count = 0;
extern void STORAGE_SD_UserInit(int use_sd_1bit);
extern int STORAGE_SD_RefreshInfo(uint32_t* block_count, int allow_fallback_to_1bit);
extern int STORAGE_SD_ReadBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
extern int STORAGE_SD_WriteBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
#else
#define STORAGE_BLK_NBR 128U
#define STORAGE_SIZE_BYTES (STORAGE_BLK_NBR * STORAGE_BLK_SIZ)
#define STORAGE_NUM_FATS 1U
#define STORAGE_ROOT_ENTRIES 16U
#define STORAGE_SECTORS_PER_FAT 1U
#define STORAGE_RESERVED_SECTORS 1U
#define STORAGE_SECTORS_PER_CLUSTER 1U
#define STORAGE_MEDIA_DESCRIPTOR 0xF8U
static uint8_t storage_data[STORAGE_SIZE_BYTES];
static uint8_t storage_initialized = 0;
#endif

static int8_t STORAGE_Init(uint8_t lun);
static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t* block_num, uint16_t* block_size);
static int8_t STORAGE_IsReady(uint8_t lun);
static int8_t STORAGE_IsWriteProtected(uint8_t lun);
static int8_t STORAGE_Read(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun(void);

#if !USE_SD_CARD
static void STORAGE_FormatIfNeeded(void);
static void WriteLe16(uint8_t* p, uint16_t v);
static void WriteLe32(uint8_t* p, uint32_t v);
#endif

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
#if USE_SD_CARD
    'R','H','Y','T','H','M',' ','S',
    'D',' ','S','T','O','R','E',' ',
#else
    'R','H','Y','T','H','M',' ','M',
    'S','C',' ','D','I','S','K',' ',
#endif
    '0','.','0','5'
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
#if USE_SD_CARD
    STORAGE_SD_UserInit(USE_SD_1BIT);
    sd_ready = 0;
    sd_block_count = 0;
    g_storage_diag_state = 1;
#else
    g_storage_diag_state = 0;
#endif
    g_storage_diag_counter = 0;
}

#if !USE_SD_CARD
static void WriteLe16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
}

static void WriteLe32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
    p[2] = (uint8_t)((v >> 16) & 0xFFU);
    p[3] = (uint8_t)((v >> 24) & 0xFFU);
}

static void STORAGE_FormatIfNeeded(void)
{
    if(storage_initialized)
        return;

    USBD_memset(storage_data, 0, STORAGE_SIZE_BYTES);

    uint8_t* boot = &storage_data[0];
    boot[0] = 0xEB;
    boot[1] = 0x3C;
    boot[2] = 0x90;
    boot[3] = 'M'; boot[4] = 'S'; boot[5] = 'D'; boot[6] = 'O';
    boot[7] = 'S'; boot[8] = '5'; boot[9] = '.'; boot[10] = '0';
    WriteLe16(&boot[11], STORAGE_BLK_SIZ);
    boot[13] = STORAGE_SECTORS_PER_CLUSTER;
    WriteLe16(&boot[14], STORAGE_RESERVED_SECTORS);
    boot[16] = STORAGE_NUM_FATS;
    WriteLe16(&boot[17], STORAGE_ROOT_ENTRIES);
    WriteLe16(&boot[19], STORAGE_BLK_NBR);
    boot[21] = STORAGE_MEDIA_DESCRIPTOR;
    WriteLe16(&boot[22], STORAGE_SECTORS_PER_FAT);
    WriteLe16(&boot[24], 1U);
    WriteLe16(&boot[26], 1U);
    WriteLe32(&boot[28], 0U);
    WriteLe32(&boot[32], 0U);
    boot[36] = 0x80;
    boot[38] = 0x29;
    WriteLe32(&boot[39], 0x12345678U);
    boot[43] = 'R'; boot[44] = 'H'; boot[45] = 'Y'; boot[46] = 'T'; boot[47] = 'H';
    boot[48] = 'M'; boot[49] = ' '; boot[50] = 'M'; boot[51] = 'S'; boot[52] = 'C'; boot[53] = ' ';
    boot[54] = 'F'; boot[55] = 'A'; boot[56] = 'T'; boot[57] = '1'; boot[58] = '2';
    boot[59] = ' '; boot[60] = ' '; boot[61] = ' ';
    boot[510] = 0x55;
    boot[511] = 0xAA;

    {
        uint8_t* fat = &storage_data[STORAGE_RESERVED_SECTORS * STORAGE_BLK_SIZ];
        fat[0] = STORAGE_MEDIA_DESCRIPTOR;
        fat[1] = 0xFF;
        fat[2] = 0xFF;
    }

    storage_initialized = 1;
}
#endif

static int8_t STORAGE_Init(uint8_t lun)
{
    UNUSED(lun);
#if USE_SD_CARD
    if(STORAGE_SD_RefreshInfo(&sd_block_count, ALLOW_SD_FALLBACK_TO_1BIT) != 0)
    {
        g_storage_diag_state = 2;
        return -1;
    }
    sd_ready = 1;
    g_storage_diag_state = 3;
    return 0;
#else
    STORAGE_FormatIfNeeded();
    return 0;
#endif
}

static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t* block_num, uint16_t* block_size)
{
    UNUSED(lun);
    *block_size = STORAGE_BLK_SIZ;
#if USE_SD_CARD
    if(STORAGE_SD_RefreshInfo(&sd_block_count, ALLOW_SD_FALLBACK_TO_1BIT) != 0)
    {
        g_storage_diag_state = 2;
        return -1;
    }
    *block_num = sd_block_count;
    g_storage_diag_state = 3;
#else
    *block_num = STORAGE_BLK_NBR;
#endif
    return 0;
}

static int8_t STORAGE_IsReady(uint8_t lun)
{
    UNUSED(lun);
#if USE_SD_CARD
    if(sd_ready)
        return 0;
    if(STORAGE_SD_RefreshInfo(&sd_block_count, ALLOW_SD_FALLBACK_TO_1BIT) != 0)
    {
        g_storage_diag_state = 2;
        return -1;
    }
    sd_ready = 1;
    g_storage_diag_state = 3;
    return 0;
#else
    STORAGE_FormatIfNeeded();
    return 0;
#endif
}

static int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
    UNUSED(lun);
    return 0;
}

static int8_t STORAGE_Read(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    UNUSED(lun);
#if USE_SD_CARD
    g_storage_diag_counter++;
    if(STORAGE_IsReady(lun) != 0)
        return -1;
    if((uint64_t)blk_addr + blk_len > sd_block_count)
    {
        g_storage_diag_state = 4;
        return -1;
    }
    if(STORAGE_SD_ReadBlocks(buf, blk_addr, blk_len) != 0)
    {
        g_storage_diag_state = 4;
        return -1;
    }
    g_storage_diag_state = 3;
    return 0;
#else
    STORAGE_FormatIfNeeded();
    {
        uint32_t offset = blk_addr * STORAGE_BLK_SIZ;
        uint32_t length = (uint32_t)blk_len * STORAGE_BLK_SIZ;
        if(offset + length > STORAGE_SIZE_BYTES)
            return -1;
        USBD_memcpy(buf, &storage_data[offset], length);
    }
    return 0;
#endif
}

static int8_t STORAGE_Write(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len)
{
    UNUSED(lun);
#if USE_SD_CARD
    g_storage_diag_counter++;
    if(STORAGE_IsReady(lun) != 0)
        return -1;
    if((uint64_t)blk_addr + blk_len > sd_block_count)
    {
        g_storage_diag_state = 4;
        return -1;
    }
    if(STORAGE_SD_WriteBlocks(buf, blk_addr, blk_len) != 0)
    {
        g_storage_diag_state = 4;
        return -1;
    }
    g_storage_diag_state = 3;
    return 0;
#else
    STORAGE_FormatIfNeeded();
    {
        uint32_t offset = blk_addr * STORAGE_BLK_SIZ;
        uint32_t length = (uint32_t)blk_len * STORAGE_BLK_SIZ;
        if(offset + length > STORAGE_SIZE_BYTES)
            return -1;
        USBD_memcpy(&storage_data[offset], buf, length);
    }
    return 0;
#endif
}

static int8_t STORAGE_GetMaxLun(void)
{
    return STORAGE_LUN_NBR - 1;
}
