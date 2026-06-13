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

#ifndef USB_COMP_AUDIO_START_ON_BOOT
#define USB_COMP_AUDIO_START_ON_BOOT 1
#endif

#ifndef USB_COMP_ENABLE_MIDI
#define USB_COMP_ENABLE_MIDI 1
#endif

#ifndef USB_COMP_TEST_MIDI
#define USB_COMP_TEST_MIDI 1
#endif

#ifndef USB_COMP_ENABLE_MSC
#define USB_COMP_ENABLE_MSC 0
#endif

#ifndef USB_COMP_TEST_MSC
#define USB_COMP_TEST_MSC 1
#endif

#ifndef USB_COMP_MSC_USE_SD
#define USB_COMP_MSC_USE_SD 0
#endif

#ifndef USB_COMP_MSC_TEST_ENABLE_DELAY_MS
#define USB_COMP_MSC_TEST_ENABLE_DELAY_MS 0
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

#if USB_COMP_ENABLE_MIDI
#include "usbd_midi.h"
#endif

#if USB_COMP_ENABLE_MSC
#include "usbd_msc.h"
#include "usbd_msc_storage.h"
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

#ifndef DSY_RAM_D2
#define DSY_RAM_D2 __attribute__((section(".heap")))
#endif

#ifndef USB_COMP_AUDIO_USE_SDRAM
#define USB_COMP_AUDIO_USE_SDRAM 0
#endif

#if USB_COMP_AUDIO_USE_SDRAM
#ifdef DSY_SDRAM_BSS
#define USB_COMP_AUDIO_BUFFER_MEM DSY_SDRAM_BSS __attribute__((aligned(32)))
#else
#define USB_COMP_AUDIO_BUFFER_MEM __attribute__((section(".sdram_bss"), aligned(32)))
#endif
#else
#define USB_COMP_AUDIO_BUFFER_MEM DSY_RAM_D2
#endif

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

#if USB_COMP_ENABLE_MIDI
uint8_t midi_class_id = kNoClass;
#endif

#if USB_COMP_ENABLE_MSC
uint8_t msc_class_id = kNoClass;
#endif

#if USB_COMP_ENABLE_HID
uint8_t hid_ep_addr[] = {0x84U};
#endif

#if USB_COMP_ENABLE_CDC
// CDC data stays on EP2.  The notification endpoint is parked on EP6 so EP3
// can carry MIDI data and EP5 can keep the large MSC IN FIFO.
uint8_t cdc_ep_addr[] = {0x82U, 0x02U, 0x86U};
#endif

#if USB_COMP_ENABLE_AUDIO
uint8_t audio_ep_addr[] = {AUDIO_OUT_EP, AUDIO_IN_EP};

#ifndef USB_COMP_AUDIO_CAPTURE_RING_SIZE
#define USB_COMP_AUDIO_CAPTURE_RING_SIZE 16384u
#endif

static_assert(USB_COMP_AUDIO_CAPTURE_RING_SIZE != 0u
                  && ((USB_COMP_AUDIO_CAPTURE_RING_SIZE
                       & (USB_COMP_AUDIO_CAPTURE_RING_SIZE - 1u))
                      == 0u),
              "USB_COMP_AUDIO_CAPTURE_RING_SIZE must be a power of two");

constexpr uint32_t kFifoRingSize = USB_COMP_AUDIO_CAPTURE_RING_SIZE;
constexpr uint32_t kFifoRingMask = kFifoRingSize - 1u;
float fifo_l[kFifoRingSize] USB_COMP_AUDIO_BUFFER_MEM;
float fifo_r[kFifoRingSize] USB_COMP_AUDIO_BUFFER_MEM;
uint32_t fifo_write_ptr = 0u;
uint32_t fifo_write_ptr_last = 0u;
float phase = 0.0f;
float phase_inc = 0.0f;
float amplitude = 0.1f;
#endif

#if USB_COMP_ENABLE_MIDI
uint8_t midi_ep_addr[] = {MIDI_IN_EP, MIDI_OUT_EP};
uint8_t midi_rx_buffer[MIDI_DATA_FS_OUT_PACKET_SIZE] DSY_RAM_D2 = {0};
#if USB_COMP_TEST_MIDI
uint8_t midi_tx_buffer[MIDI_USB_EVENT_PACKET_SIZE] DSY_RAM_D2 = {0};
#endif
int8_t MIDI_Init_HS();
int8_t MIDI_DeInit_HS();
int8_t MIDI_Receive_HS(uint8_t* buf, uint32_t* len);
int8_t MIDI_TransmitCplt_HS(uint8_t* buf, uint32_t* len, uint8_t epnum);
USBD_MIDI_ItfTypeDef USB_COMP_MIDI_Interface_fops = {
    MIDI_Init_HS,
    MIDI_DeInit_HS,
    MIDI_Receive_HS,
    MIDI_TransmitCplt_HS,
};
#endif

#if USB_COMP_ENABLE_MSC
#if !USB_COMP_ENABLE_CDC && !USB_COMP_ENABLE_HID && !USB_COMP_ENABLE_AUDIO && !USB_COMP_ENABLE_MIDI
// MSC-only mode can use EP1 like the standalone MSC example.
uint8_t msc_ep_addr[] = {0x81U, 0x01U};
#else
// Composite mode keeps MSC IN on EP5. EP6 enumerated in tests but was not
// reliable for real MSC/MIDI data on this ST full-speed stack.
uint8_t msc_ep_addr[] = {0x85U, 0x04U};
#endif
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

#if USB_COMP_ENABLE_MIDI
    USBD_RegisterClassComposite(&hUsbDeviceHS, USBD_MIDI_CLASS, CLASS_TYPE_MIDI, midi_ep_addr);
    midi_class_id = static_cast<uint8_t>(USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_MIDI, 0));
    USBD_MIDI_RegisterInterface(&hUsbDeviceHS, &USB_COMP_MIDI_Interface_fops);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

#if USB_COMP_ENABLE_MSC
    USBD_RegisterClassComposite(&hUsbDeviceHS, USBD_MSC_CLASS, CLASS_TYPE_MSC, msc_ep_addr);
    msc_class_id = static_cast<uint8_t>(USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_MSC, 0));
    USBD_MSC_RegisterStorage(&hUsbDeviceHS, &USBD_MSC_Template_fops);
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

void RunMscControl()
{
#if USB_COMP_ENABLE_CDC && USB_COMP_ENABLE_MSC
    // CDC RX only records a command byte.  The actual MSC enable/disable work
    // runs here in the main loop, away from the USB receive callback context.
    const uint8_t command = USB_COMP_CDC_TakeMscCommand();
    if(command == 0U)
        return;

    if(command == 'M')
    {
        SendCdcString(USB_COMP_MSC_Enable() == 0 ? "COMP MSC ON\r\n" : "COMP MSC ERR\r\n");
    }
    else if(command == 'm')
    {
        USB_COMP_MSC_Disable();
        SendCdcString("COMP MSC OFF\r\n");
    }
    else if(command == 'S')
    {
        SendCdcString(USB_COMP_MSC_IsEnabled() ? "COMP MSC STATUS ON\r\n" : "COMP MSC STATUS OFF\r\n");
    }
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

#if USB_COMP_ENABLE_MIDI
#if USB_COMP_TEST_MIDI
bool SendMidiPacket(uint8_t cin, uint8_t status, uint8_t data1, uint8_t data2)
{
    if(midi_class_id == kNoClass)
        return false;

    midi_tx_buffer[0] = cin;
    midi_tx_buffer[1] = status;
    midi_tx_buffer[2] = data1;
    midi_tx_buffer[3] = data2;
    if(USBD_MIDI_SetTxBuffer(&hUsbDeviceHS, midi_tx_buffer, sizeof(midi_tx_buffer), midi_class_id) != USBD_OK)
        return false;
    return USBD_MIDI_TransmitPacket(&hUsbDeviceHS, midi_class_id) == USBD_OK;
}
#endif

void RunMidiTest()
{
    // MIDI test traffic is request/response driven from MIDI_Receive_HS().
    // This avoids wedging the bulk IN endpoint before a host reader is open.
}

int8_t MIDI_Init_HS()
{
    if(midi_class_id == kNoClass)
        return USBD_FAIL;
    return USBD_MIDI_SetRxBuffer(&hUsbDeviceHS, midi_rx_buffer, midi_class_id);
}

int8_t MIDI_DeInit_HS()
{
    return USBD_OK;
}

int8_t MIDI_Receive_HS(uint8_t* buf, uint32_t* len)
{
    if(buf == nullptr || len == nullptr)
        return USBD_FAIL;

    for(uint32_t offset = 0; offset + MIDI_USB_EVENT_PACKET_SIZE <= *len; offset += MIDI_USB_EVENT_PACKET_SIZE)
    {
        const uint8_t cin = buf[offset] & 0x0F;
        const uint8_t status = buf[offset + 1];
        const uint8_t data2 = buf[offset + 3];
#if USB_COMP_TEST_MIDI
        const uint8_t data1 = buf[offset + 2];
#endif

        if((cin == 0x09U) && ((status & 0xF0U) == 0x90U) && data2 != 0U)
        {
            hw.SetLed(true);
#if USB_COMP_TEST_MIDI
            SendCdcString("COMP MIDI RX\r\n");
            SendMidiPacket(0x09, 0x90, static_cast<uint8_t>(data1 + 1U), data2);
#endif
        }
        else if((cin == 0x08U) || (((status & 0xF0U) == 0x80U) || (((status & 0xF0U) == 0x90U) && data2 == 0U)))
        {
            hw.SetLed(false);
#if USB_COMP_TEST_MIDI
            SendCdcString("COMP MIDI RX\r\n");
            SendMidiPacket(0x08, 0x80, static_cast<uint8_t>(data1 + 1U), 0);
#endif
        }
    }

    return USBD_OK;
}

int8_t MIDI_TransmitCplt_HS(uint8_t* buf, uint32_t* len, uint8_t epnum)
{
    (void)buf;
    (void)len;
    (void)epnum;
    return USBD_OK;
}
#endif

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

        const float input_l = in[0][i];
        const float input_r = in[1][i];
        const float capture_l = input_l + s;
        const float capture_r = input_r + s;

        fifo_l[fifo_write_ptr] = capture_l;
        fifo_r[fifo_write_ptr] = capture_r;
        fifo_write_ptr = (fifo_write_ptr + 1u) & kFifoRingMask;

        int16_t playback[2];
        AudioIF_PopPlaybackSamples(playback, 1);
        float pl = static_cast<float>(playback[0]) / 32767.0f;
        float pr = static_cast<float>(playback[1]) / 32767.0f;

        out[0][i] = capture_l + pl;
        out[1][i] = capture_r + pr;
    }
    fifo_write_ptr_last = fifo_write_ptr;
}
#endif
} // namespace

int main(void)
{
    hw.Init();
#if USB_COMP_ENABLE_MSC
    STORAGE_UserInit();
#endif
#if USB_COMP_ENABLE_AUDIO
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);
    InitAudioTestSignal();
#endif

    InitUSBComposite();

#if USB_COMP_ENABLE_AUDIO && USB_COMP_AUDIO_START_ON_BOOT
    // Leave this off in storage-mode builds.  Active Daisy audio DMA plus
    // SD-backed MSC is not stable yet; performance mode enables audio at boot.
    hw.StartAudio(AudioCallback);
#endif

    System::Delay(1500);

    SendHidReport();

    bool led = false;
    bool cdc_ready_sent = false;
#if USB_COMP_ENABLE_MSC && USB_COMP_MSC_TEST_ENABLE_DELAY_MS
    const uint32_t msc_test_start = System::GetNow();
    bool msc_test_enable_done = false;
#endif
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        if(!cdc_ready_sent)
            cdc_ready_sent = SendCdcString("COMP CDC NKRO ready\r\n");
        RunMscControl();
#if USB_COMP_ENABLE_MSC && USB_COMP_MSC_TEST_ENABLE_DELAY_MS
        // Diagnostic hook: proves runtime MSC enable from firmware without
        // depending on CDC being usable in every descriptor profile.
        if(!msc_test_enable_done
           && (System::GetNow() - msc_test_start) >= USB_COMP_MSC_TEST_ENABLE_DELAY_MS)
        {
            (void)USB_COMP_MSC_Enable();
            msc_test_enable_done = true;
        }
#endif
        RunCdcTest(led);
        RunHidTest();
#if USB_COMP_ENABLE_MIDI
        RunMidiTest();
#endif
        System::Delay(1000);
    }
}
