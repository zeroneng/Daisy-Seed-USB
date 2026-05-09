#include "daisy_seed.h"
#include "tusb.h"

using namespace daisy;

DaisySeed hw;
static uint32_t blink_interval_ms = 250;

extern "C" void dsy_tinyusb_irq_handler(void)
{
    tud_int_handler(0);
}

extern "C" size_t board_get_unique_id(uint8_t id[], size_t max_len)
{
    const char* serial = "DAISYCDCHID0001";
    size_t n = 15;
    if(n > max_len)
        n = max_len;
    for(size_t i = 0; i < n; i++)
        id[i] = (uint8_t)serial[i];
    return n;
}

static void led_task()
{
    static uint32_t start_ms = 0;
    static bool led_state = false;
    if(System::GetNow() - start_ms < blink_interval_ms)
        return;
    start_ms += blink_interval_ms;
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

static void hid_task()
{
    static uint32_t start_ms = 0;
    static bool pressed = false;

    if(!tud_hid_ready())
        return;
    if(System::GetNow() - start_ms < 1000)
        return;
    start_ms += 1000;

    if(!pressed)
    {
        uint8_t keycode[6] = {HID_KEY_A, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(0, 0, keycode);
        pressed = true;
        if(tud_cdc_connected())
        {
            tud_cdc_write_str("HID key down\r\n");
            tud_cdc_write_flush();
        }
    }
    else
    {
        tud_hid_keyboard_report(0, 0, NULL);
        pressed = false;
        if(tud_cdc_connected())
        {
            tud_cdc_write_str("HID key up\r\n");
            tud_cdc_write_flush();
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
extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}
extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
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
        led_task();
        cdc_task();
        hid_task();
    }
}
