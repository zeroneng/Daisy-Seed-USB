#include "usbd_msc_storage.h"

#include <string.h>

#include "daisy_core.h"

#define STORAGE_LUN_NBR 1U
#define STORAGE_BLK_SIZ 512U
#define STORAGE_RAM_BLK_NBR 128U

#ifndef USB_COMP_MSC_USE_SD
#define USB_COMP_MSC_USE_SD 0
#endif

#ifndef USB_COMP_MSC_START_ENABLED
#define USB_COMP_MSC_START_ENABLED 0
#endif

#if USB_COMP_MSC_USE_SD
/* SD-card DMA needs a cache-safe, 32-byte-aligned buffer in DMA-accessible RAM.
   The USB stack may hand us arbitrary host buffers, so SD reads/writes pass
   through this sector scratch buffer instead of touching the USB buffer
   directly. */
static uint8_t storage_sector[STORAGE_BLK_SIZ] DMA_BUFFER_MEM_SECTION __attribute__((aligned(32)));
#else
static uint8_t storage_ram_disk[STORAGE_RAM_BLK_NBR][STORAGE_BLK_SIZ] __attribute__((section(".heap"), aligned(4)));
#endif

volatile uint32_t g_msc_diag_state = 0;
volatile uint32_t g_msc_diag_counter = 0;
volatile uint32_t g_msc_runtime_enabled = 0;
volatile uint32_t g_msc_sd_ready = 0;
volatile uint32_t g_msc_sd_block_count = 0;
volatile uint32_t g_msc_last_scsi_cmd = 0;
volatile uint32_t g_msc_scsi_cmd_count = 0;

#if USB_COMP_MSC_USE_SD
extern int STORAGE_SD_Init(uint32_t* block_count);
extern int STORAGE_SD_ReadBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
extern int STORAGE_SD_WriteBlocks(uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
#endif

static int8_t STORAGE_Init(uint8_t lun);
static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t* block_num, uint16_t* block_size);
static int8_t STORAGE_IsReady(uint8_t lun);
static int8_t STORAGE_IsWriteProtected(uint8_t lun);
static int8_t STORAGE_Read(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write(uint8_t lun, uint8_t* buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun(void);
static int8_t STORAGE_HasMedium(void);
#if !USB_COMP_MSC_USE_SD
static void STORAGE_InitRamDisk(void);
#endif

static int8_t STORAGE_Inquirydata[] = {
    0x00,
#if USB_COMP_MSC_USE_SD
    0x80,
#else
    0x00,
#endif
    0x02,
    0x02,
    (STANDARD_INQUIRY_DATA_LEN - 5),
    0x00,
    0x00,
    0x00,
    'Z','E','R','O','N','E',' ',' ',
    'R','H','Y','T','H','M',' ','S',
#if USB_COMP_MSC_USE_SD
    'D',' ','S','T','O','R','E',' ',
#else
    'R','A','M',' ','M','S','C',' ',
#endif
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
    /* The MSC class may enumerate while the medium is unavailable.  That lets
       application firmware expose SD only when it enters an explicit storage
       mode. */
    g_msc_runtime_enabled = 0;
    g_msc_sd_ready = 0;
    g_msc_sd_block_count = 0;
    g_msc_diag_state = 1;

#if USB_COMP_MSC_START_ENABLED
    (void)USB_COMP_MSC_Enable();
#endif
}

int USB_COMP_MSC_Enable(void)
{
#if USB_COMP_MSC_USE_SD
    uint32_t block_count = 0;

    g_msc_runtime_enabled = 0;
    g_msc_sd_ready = 0;
    g_msc_sd_block_count = 0;

    if(STORAGE_SD_Init(&block_count) != 0 || block_count == 0U)
    {
        g_msc_diag_state = 2;
        return -1;
    }

    g_msc_sd_block_count = block_count;
    g_msc_sd_ready = 1;
#else
    STORAGE_InitRamDisk();
    g_msc_sd_block_count = STORAGE_RAM_BLK_NBR;
    g_msc_sd_ready = 1;
#endif

    g_msc_runtime_enabled = 1;
    g_msc_diag_state = 9;
    return 0;
}

void USB_COMP_MSC_Disable(void)
{
    g_msc_runtime_enabled = 0;
    g_msc_diag_state = 8;
}

uint8_t USB_COMP_MSC_IsEnabled(void)
{
    return g_msc_runtime_enabled != 0U ? 1U : 0U;
}

#if !USB_COMP_MSC_USE_SD
static void STORAGE_InitRamDisk(void)
{
    for(uint32_t block = 0; block < STORAGE_RAM_BLK_NBR; ++block)
    {
        for(uint16_t offset = 0; offset < STORAGE_BLK_SIZ; ++offset)
            storage_ram_disk[block][offset] = (uint8_t)((block + offset) & 0xFFU);
    }

    static const char label[] = "RHYTHM USB-COMP RAM MSC";
    memcpy(&storage_ram_disk[0][64], label, sizeof(label) - 1U);

    storage_ram_disk[0][510] = 0x55U;
    storage_ram_disk[0][511] = 0xAAU;
}
#endif

static int8_t STORAGE_Init(uint8_t lun)
{
    UNUSED(lun);
    if(STORAGE_HasMedium() != 0)
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
    if(STORAGE_HasMedium() != 0)
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
    if(STORAGE_HasMedium() != 0)
    {
        g_msc_diag_state = 2;
        return -1;
    }
    return 0;
}

static int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
    UNUSED(lun);
#if !USB_COMP_MSC_USE_SD
    return 0;
#else
    return 0;
#endif
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
#if USB_COMP_MSC_USE_SD
    /* Copy one sector at a time through the DMA-safe scratch buffer.  This is
       slower than direct multi-block I/O, but it avoids alignment/cache issues
       with host-provided MSC transfer buffers. */
    for(uint16_t i = 0; i < blk_len; ++i)
    {
        if(STORAGE_SD_ReadBlocks(storage_sector, blk_addr + i, 1) != 0)
        {
            g_msc_diag_state = 5;
            return -1;
        }
        for(uint16_t j = 0; j < STORAGE_BLK_SIZ; ++j)
            buf[((uint32_t)i * STORAGE_BLK_SIZ) + j] = storage_sector[j];
    }
#else
    memcpy(buf, &storage_ram_disk[blk_addr][0], (uint32_t)blk_len * STORAGE_BLK_SIZ);
#endif
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
#if USB_COMP_MSC_USE_SD
    for(uint16_t i = 0; i < blk_len; ++i)
    {
        for(uint16_t j = 0; j < STORAGE_BLK_SIZ; ++j)
            storage_sector[j] = buf[((uint32_t)i * STORAGE_BLK_SIZ) + j];
        if(STORAGE_SD_WriteBlocks(storage_sector, blk_addr + i, 1) != 0)
        {
            g_msc_diag_state = 5;
            return -1;
        }
    }
    g_msc_diag_state = 7;
    return 0;
#else
    memcpy(&storage_ram_disk[blk_addr][0], buf, (uint32_t)blk_len * STORAGE_BLK_SIZ);
    g_msc_diag_state = 7;
    return 0;
#endif
}

static int8_t STORAGE_GetMaxLun(void)
{
    return STORAGE_LUN_NBR - 1;
}

static int8_t STORAGE_HasMedium(void)
{
    /* Returning failure here makes the ST MSC/SCSI layer report no medium
       instead of exposing a half-initialized SD card to the host. */
    if(!g_msc_runtime_enabled || !g_msc_sd_ready || g_msc_sd_block_count == 0U)
        return -1;
    return 0;
}
