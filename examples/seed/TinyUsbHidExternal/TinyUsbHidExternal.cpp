#include "daisy_seed.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

using namespace daisy;

DaisySeed hw;
static uint32_t blink_interval_ms = 250;

extern "C" void dsy_tinyusb_irq_handler(void)
{
    tud_int_handler(0);
}

extern "C" size_t board_get_unique_id(uint8_t id[], size_t max_len)
{
    const char* serial = "DAISYSEEDHID01";
    size_t n = 14;
    if(n > max_len)
        n = max_len;
    for(size_t i = 0; i < n; i++)
        id[i] = (uint8_t)serial[i];
    return n;
}

extern "C" uint32_t board_button_read(void)
{
    return 1;
}

extern "C" void board_led_write(bool state)
{
    hw.SetLed(state);
}

static void led_blinking_task()
{
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if(!blink_interval_ms)
        return;
    if(System::GetNow() - start_ms < blink_interval_ms)
        return;
    start_ms += blink_interval_ms;
    hw.SetLed(led_state);
    led_state = !led_state;
}

static void hid_task(void)
{
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if(System::GetNow() - start_ms < interval_ms)
        return;
    start_ms += interval_ms;

    uint32_t const btn = board_button_read();

    if(tud_suspended() && btn)
    {
        tud_remote_wakeup();
    }
    else
    {
        if(tud_hid_n_ready(ITF_NUM_KEYBOARD))
        {
            static bool has_keyboard_key = false;
            uint8_t const report_id = 0;
            uint8_t const modifier = 0;

            if(btn)
            {
                uint8_t keycode[6] = {0};
                keycode[0] = HID_KEY_A;
                tud_hid_n_keyboard_report(ITF_NUM_KEYBOARD, report_id, modifier, keycode);
                has_keyboard_key = true;
            }
            else
            {
                if(has_keyboard_key)
                    tud_hid_n_keyboard_report(ITF_NUM_KEYBOARD, report_id, modifier, NULL);
                has_keyboard_key = false;
            }
        }
    }
}

extern "C" void tud_mount_cb(void) { blink_interval_ms = 1000; }
extern "C" void tud_umount_cb(void) { blink_interval_ms = 250; }
extern "C" void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    blink_interval_ms = 2500;
}
extern "C" void tud_resume_cb(void)
{
    blink_interval_ms = tud_mounted() ? 1000 : 250;
}
extern "C" void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol)
{
    (void)instance;
    (void)protocol;
}
extern "C" void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void)instance;
    (void)report;
    (void)len;
}
extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}
extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
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
        hid_task();
    }
}
