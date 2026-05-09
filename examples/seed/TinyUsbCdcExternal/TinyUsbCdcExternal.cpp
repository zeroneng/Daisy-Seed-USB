#include "daisy_seed.h"
#include "tusb.h"

using namespace daisy;

DaisySeed hw;

extern "C" void dsy_tinyusb_irq_handler(void)
{
    tud_int_handler(0);
}

extern "C" size_t board_get_unique_id(uint8_t id[], size_t max_len)
{
    const char* serial = "DAISYSEEDCDC01";
    size_t n = 14;
    if(n > max_len)
        n = max_len;
    for(size_t i = 0; i < n; i++)
        id[i] = (uint8_t)serial[i];
    return n;
}

static void led_blinking_task()
{
    static uint32_t start_ms = 0;
    static bool led_state = false;
    uint32_t interval = tud_mounted() ? 1000 : 250;

    if(System::GetNow() - start_ms < interval)
        return;
    start_ms += interval;
    hw.SetLed(led_state);
    led_state = !led_state;
}

static void cdc_task()
{
    if(tud_cdc_available())
    {
        char buf[64];
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        tud_cdc_write(buf, count);
        tud_cdc_write_flush();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.usb_handle.Init(UsbHandle::FS_EXTERNAL);

    tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
    tusb_init(0, &dev_init);

    while(1)
    {
        tud_task();
        led_blinking_task();
        cdc_task();
    }
}
