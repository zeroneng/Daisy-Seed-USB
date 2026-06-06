/**
  ******************************************************************************
  * @file    usbd_conf.c
  * @brief   USB Device board support / PCD glue layer for Daisy Seed UAC1
  *
  * Changes from libDaisy CDC baseline:
  *  1. Sof_enable = ENABLE  — audio class USBD_AUDIO_SOF() needs SOF events
  *  2. TX FIFO 1 enlarged    — audio IN packets are 192 bytes; old 0x80 (128B)
  *                             was borderline; 0xC0 (192B) gives clean headroom
  *  3. vbus_sensing_enable   — left ENABLE for FS (Daisy has VBUS sense on PA9)
  *  4. HS init removed       — Daisy Seed only has the FS connector exposed;
  *                             keep the HS block for completeness but it's unused
  ******************************************************************************
  */

#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
#include "usbd_def.h"
#include "usbd_core.h"

PCD_HandleTypeDef hpcd_USB_OTG_FS;
PCD_HandleTypeDef hpcd_USB_OTG_HS;

void Error_Handler(void) { while(1) {} }

USBD_StatusTypeDef USBD_Get_USB_Status(HAL_StatusTypeDef hal_status);

/* -------------------------------------------------------------------------- */
/* MSP Init / DeInit                                                           */
/* -------------------------------------------------------------------------- */

void HAL_PCD_MspInit(PCD_HandleTypeDef *pcdHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(pcdHandle->Instance == USB_OTG_FS)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA11 = DM, PA12 = DP, PA9 = VBUS (unused in external build) */
        GPIO_InitStruct.Pin       = GPIO_PIN_12 | GPIO_PIN_11;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* VBUS sense (input, no pull) */
        GPIO_InitStruct.Pin  = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

        /* Priority 0 so audio IRQs aren't delayed */
        HAL_NVIC_SetPriority(OTG_FS_EP1_OUT_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(OTG_FS_EP1_OUT_IRQn);
        HAL_NVIC_SetPriority(OTG_FS_EP1_IN_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(OTG_FS_EP1_IN_IRQn);
        HAL_NVIC_SetPriority(OTG_FS_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
    }
    else if(pcdHandle->Instance == USB_OTG_HS)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin       = GPIO_PIN_14 | GPIO_PIN_15;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF12_OTG2_FS;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
        HAL_NVIC_SetPriority(OTG_HS_EP1_OUT_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(OTG_HS_EP1_OUT_IRQn);
        HAL_NVIC_SetPriority(OTG_HS_EP1_IN_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(OTG_HS_EP1_IN_IRQn);
        HAL_NVIC_SetPriority(OTG_HS_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    }
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *pcdHandle)
{
    if(pcdHandle->Instance == USB_OTG_FS)
    {
        __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_9);
        HAL_NVIC_DisableIRQ(OTG_FS_EP1_OUT_IRQn);
        HAL_NVIC_DisableIRQ(OTG_FS_EP1_IN_IRQn);
        HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    }
    else if(pcdHandle->Instance == USB_OTG_HS)
    {
        __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_14 | GPIO_PIN_15);
        HAL_NVIC_DisableIRQ(OTG_HS_EP1_OUT_IRQn);
        HAL_NVIC_DisableIRQ(OTG_HS_EP1_IN_IRQn);
        HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
    }
}

/* -------------------------------------------------------------------------- */
/* PCD → USB Device Library callbacks                                         */
/* -------------------------------------------------------------------------- */

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetupStage((USBD_HandleTypeDef *)hpcd->pData, (uint8_t *)hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataOutStage((USBD_HandleTypeDef *)hpcd->pData, epnum,
                          hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataInStage((USBD_HandleTypeDef *)hpcd->pData, epnum,
                         hpcd->IN_ep[epnum].xfer_buff);
}

/* SOF is essential for the audio class — it drives the periodic transfer
   callback that feeds the isochronous IN endpoint with new audio data. */
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SOF((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_SpeedTypeDef speed = USBD_SPEED_FULL;
    if(hpcd->Init.speed == PCD_SPEED_HIGH)       speed = USBD_SPEED_HIGH;
    else if(hpcd->Init.speed == PCD_SPEED_FULL)  speed = USBD_SPEED_FULL;
    else                                          Error_Handler();
    USBD_LL_SetSpeed((USBD_HandleTypeDef *)hpcd->pData, speed);
    USBD_LL_Reset((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_Suspend((USBD_HandleTypeDef *)hpcd->pData);
    __HAL_PCD_GATE_PHYCLOCK(hpcd);
    if(hpcd->Init.low_power_enable)
    {
        SCB->SCR |= (uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);
    }
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_Resume((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_DevConnected((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_DevDisconnected((USBD_HandleTypeDef *)hpcd->pData);
}

/* -------------------------------------------------------------------------- */
/* USB Device Library → PCD (LL driver interface)                             */
/* -------------------------------------------------------------------------- */

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    if(pdev->id == DEVICE_FS)
    {
        hpcd_USB_OTG_FS.pData = pdev;
        pdev->pData           = &hpcd_USB_OTG_FS;

        hpcd_USB_OTG_FS.Instance                     = USB_OTG_FS;
        hpcd_USB_OTG_FS.Init.dev_endpoints           = 9;
        hpcd_USB_OTG_FS.Init.speed                   = PCD_SPEED_FULL;
        hpcd_USB_OTG_FS.Init.dma_enable              = DISABLE;
        hpcd_USB_OTG_FS.Init.phy_itface              = PCD_PHY_EMBEDDED;

        /* ENABLE SOF — the USBD_AUDIO_SOF() callback drives the periodic
           audio data feed on the isochronous IN endpoint.  Without SOF,
           audio streaming will not start. */
        hpcd_USB_OTG_FS.Init.Sof_enable              = ENABLE;

        hpcd_USB_OTG_FS.Init.low_power_enable        = DISABLE;
        hpcd_USB_OTG_FS.Init.lpm_enable              = DISABLE;
        hpcd_USB_OTG_FS.Init.battery_charging_enable = ENABLE;
        hpcd_USB_OTG_FS.Init.vbus_sensing_enable     = ENABLE;
        hpcd_USB_OTG_FS.Init.use_dedicated_ep1       = DISABLE;

        if(HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) { Error_Handler(); }

#if USB_COMP_ENABLE_MSC
#if !USB_COMP_ENABLE_CDC && !USB_COMP_ENABLE_HID && !USB_COMP_ENABLE_AUDIO && !USB_COMP_ENABLE_MIDI
        /* MSC-only mirrors the simpler standalone storage topology: EP1 gets
           enough FIFO for bulk transfers without sharing space with audio,
           MIDI, HID, or CDC. */
        HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x80);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x80);
#else
        /* Full composite FIFO allocation (320 words total):
             Rx FIFO   = 0x4C words — shared OUT/control receive FIFO
             TX0 FIFO  = 0x10 words — EP0 control
             TX1 FIFO  = 0x30 words — audio capture IN
             TX2 FIFO  = 0x10 words — CDC data IN
             TX3 FIFO  = 0x04 words — MIDI data IN
             TX4 FIFO  = 0x10 words — HID keyboard IN
             TX5 FIFO  = 0x80 words — MSC bulk IN
             TX6 FIFO  = 0x10 words — CDC notification IN
           Total used = 0x140 = 320 words.
           Keep EP6 for CDC notification only; testing showed it is not a
           trustworthy real-data endpoint for MSC or MIDI in this setup.
        */
        HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x4C);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x10);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x30);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 2, 0x10);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 3, 0x04);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 4, 0x10);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 5, 0x80);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 6, 0x10);
#endif
#else
        /* FIFO allocation (in 32-bit words, total FS FIFO = 320 words = 1280 B):
             Rx FIFO   = 0x5C words — shared OUT/control receive FIFO
             TX0 FIFO  = 0x10 words — EP0 control
             TX1 FIFO  = 0xA0 words — EP1 IN audio capture
             TX2 FIFO  = 0x10 words — CDC data IN
             TX3 FIFO  = 0x04 words — CDC command IN
             TX4 FIFO  = 0x10 words — HID keyboard IN
             TX5 FIFO  = 0x10 words — MIDI data IN
           Total used = 0x140 = 320 words. The larger Rx FIFO gives CDC, audio,
           and MIDI OUT transfers enough shared receive space.
        */
        HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x5C);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x10);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0xA0);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 2, 0x10);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 3, 0x04);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 4, 0x10);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 5, 0x10);
#endif
    }

    if(pdev->id == DEVICE_HS)
    {
        hpcd_USB_OTG_HS.pData = pdev;
        pdev->pData           = &hpcd_USB_OTG_HS;

        hpcd_USB_OTG_HS.Instance                     = USB_OTG_HS;
        hpcd_USB_OTG_HS.Init.dev_endpoints           = 9;
        hpcd_USB_OTG_HS.Init.speed                   = PCD_SPEED_FULL;
        hpcd_USB_OTG_HS.Init.dma_enable              = DISABLE;
        hpcd_USB_OTG_HS.Init.phy_itface              = USB_OTG_EMBEDDED_PHY;
        hpcd_USB_OTG_HS.Init.Sof_enable              = ENABLE;
        hpcd_USB_OTG_HS.Init.low_power_enable        = DISABLE;
        hpcd_USB_OTG_HS.Init.lpm_enable              = DISABLE;
        hpcd_USB_OTG_HS.Init.battery_charging_enable = ENABLE;
        hpcd_USB_OTG_HS.Init.vbus_sensing_enable     = DISABLE;
        hpcd_USB_OTG_HS.Init.use_dedicated_ep1       = DISABLE;
        hpcd_USB_OTG_HS.Init.use_external_vbus       = DISABLE;

        if(HAL_PCD_Init(&hpcd_USB_OTG_HS) != HAL_OK) { Error_Handler(); }

        HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x80);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, 0x40);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, 0xC0);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, 0x40);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 3, 0x08);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 4, 0x10);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 5, 0x10);
    }

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
    return USBD_Get_USB_Status(HAL_PCD_DeInit(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
    return USBD_Get_USB_Status(HAL_PCD_Start(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
    return USBD_Get_USB_Status(HAL_PCD_Stop(pdev->pData));
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev,
                                   uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type));
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Close(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Flush(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_SetStall(pdev->pData, ep_addr));
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_ClrStall(pdev->pData, ep_addr));
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef *)pdev->pData;
    if((ep_addr & 0x80) == 0x80)
        return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
    else
        return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
    return USBD_Get_USB_Status(HAL_PCD_SetAddress(pdev->pData, dev_addr));
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev,
                                     uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, (uint16_t)size));
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev,
                                          uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, (uint16_t)size));
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef *)pdev->pData, ep_addr);
}

void USBD_LL_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

USBD_StatusTypeDef USBD_Get_USB_Status(HAL_StatusTypeDef hal_status)
{
    switch(hal_status)
    {
        case HAL_OK:      return USBD_OK;
        case HAL_ERROR:   return USBD_FAIL;
        case HAL_BUSY:    return USBD_BUSY;
        case HAL_TIMEOUT: return USBD_FAIL;
        default:          return USBD_FAIL;
    }
}
