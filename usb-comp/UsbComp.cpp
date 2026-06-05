#include "daisy_seed.h"

#include <math.h>
#include <cstring>

using namespace daisy;

#ifndef USB_COMP_ENABLE_CDC
#define USB_COMP_ENABLE_CDC 1
#endif

#ifndef USB_COMP_ENABLE_HID
#define USB_COMP_ENABLE_HID 1
#endif

#ifndef USB_COMP_TEST_CDC
#define USB_COMP_TEST_CDC 1
#endif

#ifndef USB_COMP_TEST_HID
#define USB_COMP_TEST_HID 1
#endif

#ifndef USB_COMP_ENABLE_AUDIO
#define USB_COMP_ENABLE_AUDIO 1
#endif

#ifndef USB_COMP_TEST_AUDIO
#define USB_COMP_TEST_AUDIO 1
#endif

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_def.h"
#include "usb_comp_cdc_if.h"

#if USB_COMP_ENABLE_CDC
#include "usbd_cdc.h"
#endif

#if USB_COMP_ENABLE_HID
#include "usbd_hid.h"
#endif

#if USB_COMP_ENABLE_AUDIO
#include "audio_fifo_shared.h"
#include "usbd_audio.h"
#include "usbd_audio_if.h"
#endif

extern USBD_HandleTypeDef hUsbDeviceHS;
extern USBD_ClassTypeDef USBD_CMPSIT;
uint32_t USBD_CMPSIT_SetClassID(USBD_HandleTypeDef *pdev,
                                USBD_CompositeClassTypeDef Class,
                                uint32_t Instance);
uint32_t USBD_CMPSIT_GetClassID(USBD_HandleTypeDef *pdev,
                                USBD_CompositeClassTypeDef Class,
                                uint32_t Instance);
}

namespace {
DaisySeed hw;

#ifndef M_TWOPI
#define M_TWOPI 6.28318530717958647692f
#endif

constexpr uint8_t kNoClass = 0xFFU;

#if USB_COMP_ENABLE_HID
constexpr uint8_t kNkroReportBytes = 33;
#if USB_COMP_TEST_HID
constexpr uint8_t kNkroBitmapOffset = 1;
constexpr uint8_t kHidKeyA = 0x04;
#endif

uint8_t hid_report[kNkroReportBytes] = {0};
uint8_t hid_class_id = kNoClass;
#endif

#if USB_COMP_ENABLE_CDC
uint8_t cdc_class_id = kNoClass;
#endif

#if USB_COMP_ENABLE_AUDIO
uint8_t audio_class_id = kNoClass;
#endif

#if USB_COMP_ENABLE_HID
uint8_t hid_ep_addr[] = {0x84U};
#endif

#if USB_COMP_ENABLE_CDC
uint8_t cdc_ep_addr[] = {0x82U, 0x02U, 0x83U};
#endif

#if USB_COMP_ENABLE_AUDIO
#define DSY_RAM_D2 __attribute__((section(".heap")))
uint8_t audio_ep_addr[] = {AUDIO_OUT_EP, AUDIO_IN_EP};

constexpr uint32_t kFifoRingSize = 16384u;
constexpr uint32_t kFifoRingMask = kFifoRingSize - 1u;
float fifo_l[kFifoRingSize] DSY_RAM_D2;
float fifo_r[kFifoRingSize] DSY_RAM_D2;
uint32_t fifo_write_ptr = 0u;
uint32_t fifo_write_ptr_last = 0u;
float phase = 0.0f;
float phase_inc = 0.0f;
float amplitude = 0.1f;
#endif

#if USB_COMP_ENABLE_AUDIO
extern "C" uint32_t AudioFifo_GetWritePtrLast(void)
{
    return fifo_write_ptr_last;
}

extern "C" float AudioFifo_GetLeft(uint32_t index)
{
    return fifo_l[index & kFifoRingMask];
}

extern "C" float AudioFifo_GetRight(uint32_t index)
{
    return fifo_r[index & kFifoRingMask];
}

extern "C" uint32_t AudioFifo_GetRingSize(void)
{
    return kFifoRingSize;
}

extern "C" uint32_t AudioFifo_GetRingMask(void)
{
    return kFifoRingMask;
}
#endif

#if USB_COMP_ENABLE_HID && USB_COMP_TEST_HID
void SetKeyState(uint8_t keycode, bool pressed)
{
    if(keycode > 0xE7)
        return;

    if(keycode >= 0xE0 && keycode <= 0xE7)
    {
        const uint8_t bit_mask = static_cast<uint8_t>(1u << (keycode - 0xE0));
        if(pressed)
            hid_report[0] |= bit_mask;
        else
            hid_report[0] &= static_cast<uint8_t>(~bit_mask);
        return;
    }

    const uint8_t byte_index = static_cast<uint8_t>(keycode >> 3);
    if(byte_index >= (kNkroReportBytes - kNkroBitmapOffset))
        return;

    const uint8_t bit_mask = static_cast<uint8_t>(1u << (keycode & 0x07u));
    uint8_t& byte = hid_report[kNkroBitmapOffset + byte_index];
    if(pressed)
        byte |= bit_mask;
    else
        byte &= static_cast<uint8_t>(~bit_mask);
}
#endif

void InitUSBComposite()
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);

#if USB_COMP_ENABLE_HID
    USBD_RegisterClassComposite(&hUsbDeviceHS, USBD_HID_CLASS, CLASS_TYPE_HID, hid_ep_addr);
    hid_class_id = static_cast<uint8_t>(USBD_CMPSIT_GetClassID(&hUsbDeviceHS, CLASS_TYPE_HID, 0));
#endif

#if USB_COMP_ENABLE_CDC
    USBD_RegisterClassComposite(&hUsbDeviceHS, USBD_CDC_CLASS, CLASS_TYPE_CDC, cdc_ep_addr);
    cdc_class_id = static_cast<uint8_t>(USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_CDC, 0));
    USB_COMP_CDC_SetClassId(cdc_class_id);
    USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USB_COMP_CDC_Interface_fops_HS);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

#if USB_COMP_ENABLE_AUDIO
    USBD_RegisterClassComposite(&hUsbDeviceHS, USBD_AUDIO_CLASS, CLASS_TYPE_AUDIO, audio_ep_addr);
    audio_class_id = static_cast<uint8_t>(USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_AUDIO, 0));
    USBD_AUDIO_RegisterInterface(&hUsbDeviceHS, &USBD_AUDIO_fops);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

    USBD_Start(&hUsbDeviceHS);
}

bool SendCdcString(const char* s)
{
#if USB_COMP_ENABLE_CDC
    if(!s || cdc_class_id == kNoClass || !USB_COMP_CDC_IsConnected())
        return false;

    return CDC_Transmit_HS((uint8_t*)s, (uint16_t)strlen(s)) == USBD_OK;
#else
    (void)s;
    return false;
#endif
}

void SendHidReport()
{
#if USB_COMP_ENABLE_HID
    if(hid_class_id == kNoClass)
        return;

    USBD_HID_SendReport(&hUsbDeviceHS, hid_report, sizeof(hid_report), hid_class_id);
#endif
}

void RunCdcTest(bool led)
{
#if USB_COMP_ENABLE_CDC && USB_COMP_TEST_CDC
    SendCdcString(led ? "COMP CDC LED ON\r\n" : "COMP CDC LED OFF\r\n");
#else
    (void)led;
#endif
}

void RunHidTest()
{
#if USB_COMP_ENABLE_HID && USB_COMP_TEST_HID
    std::memset(hid_report, 0, sizeof(hid_report));
    SetKeyState(kHidKeyA, true);
    SendHidReport();
    System::Delay(40);
    SetKeyState(kHidKeyA, false);
    SendHidReport();
#endif
}

#if USB_COMP_ENABLE_AUDIO
void InitAudioTestSignal()
{
    phase = 0.0f;
    phase_inc = M_TWOPI * 100.0f / 48000.0f;
    amplitude = 0.1f;
    fifo_write_ptr = 0u;
    fifo_write_ptr_last = 0u;
    for(uint32_t i = 0; i < kFifoRingSize; i++)
    {
        fifo_l[i] = 0.0f;
        fifo_r[i] = 0.0f;
    }
}

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    (void)in;
    for(size_t i = 0; i < size; i++)
    {
#if USB_COMP_TEST_AUDIO
        float s = sinf(phase) * amplitude;
        phase += phase_inc;
        if(phase >= M_TWOPI)
            phase -= M_TWOPI;
#else
        float s = 0.0f;
#endif

        fifo_l[fifo_write_ptr] = s;
        fifo_r[fifo_write_ptr] = s;
        fifo_write_ptr = (fifo_write_ptr + 1u) & kFifoRingMask;

        int16_t playback[2];
        AudioIF_PopPlaybackSamples(playback, 1);
        float pl = static_cast<float>(playback[0]) / 32767.0f;
        float pr = static_cast<float>(playback[1]) / 32767.0f;

        out[0][i] = s + pl;
        out[1][i] = s + pr;
    }
    fifo_write_ptr_last = fifo_write_ptr;
}
#endif
} // namespace

int main(void)
{
    hw.Init();
#if USB_COMP_ENABLE_AUDIO
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);
    InitAudioTestSignal();
#endif

    InitUSBComposite();

#if USB_COMP_ENABLE_AUDIO
    hw.StartAudio(AudioCallback);
#endif

    System::Delay(1500);

    SendHidReport();

    bool led = false;
    bool cdc_ready_sent = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        if(!cdc_ready_sent)
            cdc_ready_sent = SendCdcString("COMP CDC NKRO ready\r\n");
        RunCdcTest(led);
        RunHidTest();
        System::Delay(1000);
    }
}
