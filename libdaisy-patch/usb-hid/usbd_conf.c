/**
  ******************************************************************************
  * @file    usbd_conf.c
  * @brief   USB Device board support / PCD glue layer for Daisy Seed HID sample
  *
  * Notes for this sample:
  *  1. Sof_enable = ENABLE to preserve standard USB device timing callbacks
  *  2. FIFO sizes are set conservatively for this sample and can be tuned later
  *  3. vbus_sensing_enable remains ENABLE for FS (Daisy has VBUS sense on PA9)
  *  4. HS init block is kept for completeness, though Daisy Seed typically uses FS
  ******************************************************************************
  */

#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
#include "usbd_def.h"
#include "usbd_core.h"

USBD_HandleTypeDef hUsbDeviceHS;
USBD_HandleTypeDef hUsbDeviceFS;

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

        /* PA11 = DM, PA12 = DP, PA9 = VBUS sense */
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

        /* USB IRQ priority */
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

/* SOF callback is forwarded into the USB device stack. */
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

        /* Keep SOF enabled for normal USB device timing/callback flow. */
        hpcd_USB_OTG_FS.Init.Sof_enable              = ENABLE;

        hpcd_USB_OTG_FS.Init.low_power_enable        = DISABLE;
        hpcd_USB_OTG_FS.Init.lpm_enable              = DISABLE;
        hpcd_USB_OTG_FS.Init.battery_charging_enable = ENABLE;
        hpcd_USB_OTG_FS.Init.vbus_sensing_enable     = ENABLE;
        hpcd_USB_OTG_FS.Init.use_dedicated_ep1       = DISABLE;

        if(HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) { Error_Handler(); }

        /* FIFO allocation (in 32-bit words, total FS FIFO = 320 words = 1280 B).
           These values are conservative defaults and can be adjusted if the
           endpoint layout changes. */
        HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x80);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0xC0);
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

        HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x200);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, 0x80);
        HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, 0x174);
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
                                     uint8_t ep_addr, uint8_t *pbuf, uint16_t size)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size));
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev,
                                           uint8_t ep_addr, uint8_t *pbuf, uint16_t size)
{
    return USBD_Get_USB_Status(HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size));
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
