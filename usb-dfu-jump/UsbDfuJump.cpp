#include "daisy_seed.h"

using namespace daisy;

extern "C" {
#include "stm32h7xx.h"
}

static void JumpToSystemBootloader()
{
    __disable_irq();

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    for(int i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    __DSB();
    __ISB();

    constexpr uint32_t bootloader_base = 0x1FF09800;
    uint32_t boot_stack = *(__IO uint32_t *)bootloader_base;
    uint32_t boot_entry = *(__IO uint32_t *)(bootloader_base + 4U);

    typedef void (*BootJumpFunc)(void);
    BootJumpFunc jump = (BootJumpFunc)boot_entry;

    __set_MSP(boot_stack);
    __DSB();
    __ISB();
    jump();

    while(1) {}
}

int main(void)
{
    DaisySeed hw;
    hw.Init();
    System::Delay(100);
    JumpToSystemBootloader();

    while(1) {}
}
