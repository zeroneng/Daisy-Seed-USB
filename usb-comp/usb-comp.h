#pragma once

// Header-only application bridge for usb-comp.
//
// Include this from exactly one C++ translation unit, normally Main.cpp.
// It defines the AudioFifo_* C symbols consumed by usbd_audio_if.c.

#include <stdint.h>

#include <cstring>

#ifndef USBD_CMPSIT_ACTIVATE_CDC
#define USBD_CMPSIT_ACTIVATE_CDC 1
#endif

#ifndef USBD_CMPSIT_ACTIVATE_HID
#define USBD_CMPSIT_ACTIVATE_HID 1
#endif

#ifndef USBD_CMPSIT_ACTIVATE_AUDIO
#define USBD_CMPSIT_ACTIVATE_AUDIO 1
#endif

#ifndef USBD_CMPSIT_ACTIVATE_MIDI
#define USBD_CMPSIT_ACTIVATE_MIDI 1
#endif

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_def.h"

#if USBD_CMPSIT_ACTIVATE_CDC
#include "usb_comp_cdc_if.h"
#include "usbd_cdc.h"
#endif

#if USBD_CMPSIT_ACTIVATE_HID
#include "usbd_hid.h"
#endif

#if USBD_CMPSIT_ACTIVATE_AUDIO
#include "audio_fifo_shared.h"
#include "usbd_audio.h"
#include "usbd_audio_if.h"
#endif

#if USBD_CMPSIT_ACTIVATE_MIDI
#include "usbd_midi.h"
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

#ifndef DSY_RAM_D2
#define DSY_RAM_D2 __attribute__((section(".heap")))
#endif

#ifndef USB_COMP_DEBUG_FAIL
#define USB_COMP_DEBUG_FAIL(code) do {} while(0)
#endif

namespace UsbComp
{
constexpr uint8_t kNoClass = 0xFFU;

#if USBD_CMPSIT_ACTIVATE_CDC
static uint8_t cdc_class_id = kNoClass;
static uint8_t cdc_ep_addr[] = {0x82U, 0x02U, 0x86U};
#endif

#if USBD_CMPSIT_ACTIVATE_HID
constexpr uint8_t kNkroReportBytes = 33;
constexpr uint8_t kNkroBitmapOffset = 1;
constexpr uint8_t kHidKeyA = 0x04;
static uint8_t hid_class_id = kNoClass;
static uint8_t hid_ep_addr[] = {0x84U};
static uint8_t hid_report[kNkroReportBytes] = {};
static uint8_t hid_last_sent_report[kNkroReportBytes] = {};
#endif

#if USBD_CMPSIT_ACTIVATE_AUDIO
static uint8_t audio_class_id = kNoClass;
static uint8_t audio_ep_addr[] = {AUDIO_OUT_EP, AUDIO_IN_EP};

#ifndef USB_COMP_AUDIO_CAPTURE_RING_SIZE
#define USB_COMP_AUDIO_CAPTURE_RING_SIZE 512u
#endif

static_assert(USB_COMP_AUDIO_CAPTURE_RING_SIZE >= 128u
                  && ((USB_COMP_AUDIO_CAPTURE_RING_SIZE
                       & (USB_COMP_AUDIO_CAPTURE_RING_SIZE - 1u))
                      == 0u),
              "USB_COMP_AUDIO_CAPTURE_RING_SIZE must be a power of two >= 128");

constexpr uint32_t kAudioRingSize = USB_COMP_AUDIO_CAPTURE_RING_SIZE;
constexpr uint32_t kAudioRingMask = kAudioRingSize - 1u;

static float audio_fifo_l[kAudioRingSize] DSY_RAM_D2 = {};
static float audio_fifo_r[kAudioRingSize] DSY_RAM_D2 = {};
static uint32_t audio_fifo_write_ptr = 0u;
static uint32_t audio_fifo_write_ptr_last = 0u;
#endif

#if USBD_CMPSIT_ACTIVATE_MIDI
static uint8_t midi_class_id = kNoClass;
static uint8_t midi_ep_addr[] = {MIDI_IN_EP, MIDI_OUT_EP};
static uint8_t midi_rx_buffer[MIDI_DATA_FS_OUT_PACKET_SIZE] DSY_RAM_D2 = {};
static uint8_t midi_tx_buffer[MIDI_USB_EVENT_PACKET_SIZE] DSY_RAM_D2 = {};
static uint8_t midi_pending_packet[MIDI_USB_EVENT_PACKET_SIZE] = {};
static volatile bool midi_tx_pending = false;

static int8_t MidiInit();
static int8_t MidiDeInit();
static int8_t MidiReceive(uint8_t* buf, uint32_t* len);
static int8_t MidiTransmitComplete(uint8_t* buf, uint32_t* len, uint8_t epnum);

static USBD_MIDI_ItfTypeDef midi_fops = {
    MidiInit,
    MidiDeInit,
    MidiReceive,
    MidiTransmitComplete,
};
#endif

static void Init()
{
#if USBD_CMPSIT_ACTIVATE_AUDIO
    audio_fifo_write_ptr = 0u;
    audio_fifo_write_ptr_last = 0u;
    for(uint32_t i = 0; i < kAudioRingSize; i++)
    {
        audio_fifo_l[i] = 0.0f;
        audio_fifo_r[i] = 0.0f;
    }
#endif

    if(USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS) != USBD_OK)
        USB_COMP_DEBUG_FAIL(1);

#if USBD_CMPSIT_ACTIVATE_HID
    if(USBD_RegisterClassComposite(&hUsbDeviceHS,
                                   USBD_HID_CLASS,
                                   CLASS_TYPE_HID,
                                   hid_ep_addr)
       != USBD_OK)
        USB_COMP_DEBUG_FAIL(2);
    hid_class_id = static_cast<uint8_t>(
        USBD_CMPSIT_GetClassID(&hUsbDeviceHS, CLASS_TYPE_HID, 0));
    if(hid_class_id == kNoClass)
        USB_COMP_DEBUG_FAIL(3);
#endif

#if USBD_CMPSIT_ACTIVATE_CDC
    if(USBD_RegisterClassComposite(&hUsbDeviceHS,
                                   USBD_CDC_CLASS,
                                   CLASS_TYPE_CDC,
                                   cdc_ep_addr)
       != USBD_OK)
        USB_COMP_DEBUG_FAIL(4);
    cdc_class_id = static_cast<uint8_t>(
        USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_CDC, 0));
    if(cdc_class_id == kNoClass)
        USB_COMP_DEBUG_FAIL(5);
    USB_COMP_CDC_SetClassId(cdc_class_id);
    if(USBD_CDC_RegisterInterface(&hUsbDeviceHS,
                                  &USB_COMP_CDC_Interface_fops_HS)
       != USBD_OK)
        USB_COMP_DEBUG_FAIL(6);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

#if USBD_CMPSIT_ACTIVATE_AUDIO
    if(USBD_RegisterClassComposite(&hUsbDeviceHS,
                                   USBD_AUDIO_CLASS,
                                   CLASS_TYPE_AUDIO,
                                   audio_ep_addr)
       != USBD_OK)
        USB_COMP_DEBUG_FAIL(7);
    audio_class_id = static_cast<uint8_t>(
        USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_AUDIO, 0));
    if(audio_class_id == kNoClass)
        USB_COMP_DEBUG_FAIL(8);
    if(USBD_AUDIO_RegisterInterface(&hUsbDeviceHS, &USBD_AUDIO_fops)
       != USBD_OK)
        USB_COMP_DEBUG_FAIL(9);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

#if USBD_CMPSIT_ACTIVATE_MIDI
    if(USBD_RegisterClassComposite(&hUsbDeviceHS,
                                   USBD_MIDI_CLASS,
                                   CLASS_TYPE_MIDI,
                                   midi_ep_addr)
       != USBD_OK)
        USB_COMP_DEBUG_FAIL(10);
    midi_class_id = static_cast<uint8_t>(
        USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_MIDI, 0));
    if(midi_class_id == kNoClass)
        USB_COMP_DEBUG_FAIL(11);
    if(USBD_MIDI_RegisterInterface(&hUsbDeviceHS, &midi_fops) != USBD_OK)
        USB_COMP_DEBUG_FAIL(12);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

    if(USBD_Start(&hUsbDeviceHS) != USBD_OK)
        USB_COMP_DEBUG_FAIL(13);
}

#if USBD_CMPSIT_ACTIVATE_AUDIO
static void PushCapture(float left, float right)
{
    const uint32_t write_idx = audio_fifo_write_ptr & kAudioRingMask;
    audio_fifo_l[write_idx] = left;
    audio_fifo_r[write_idx] = right;
    audio_fifo_write_ptr++;
}

static void CommitCaptureBlock()
{
    audio_fifo_write_ptr_last = audio_fifo_write_ptr;
}

static void PopPlayback(float& left, float& right)
{
    int16_t samples[2] = {0, 0};
    AudioIF_PopPlaybackSamples(samples, 1);
    left = static_cast<float>(samples[0]) / 32767.0f;
    right = static_cast<float>(samples[1]) / 32767.0f;
}
#else
static void PushCapture(float, float) {}
static void CommitCaptureBlock() {}
static void PopPlayback(float& left, float& right)
{
    left = 0.0f;
    right = 0.0f;
}
#endif

#if USBD_CMPSIT_ACTIVATE_CDC
static inline bool SendCdc(const char* text)
{
    if(text == nullptr || cdc_class_id == kNoClass || !USB_COMP_CDC_IsConnected())
        return false;

    uint16_t len = 0;
    while(text[len] != '\0')
        len++;

    return CDC_Transmit_HS((uint8_t*)text, len) == USBD_OK;
}
#else
static inline bool SendCdc(const char*) { return false; }
#endif

#if USBD_CMPSIT_ACTIVATE_HID
static bool HidReportChanged()
{
    return std::memcmp(hid_report, hid_last_sent_report, sizeof(hid_report)) != 0;
}

static void SaveHidReport()
{
    std::memcpy(hid_last_sent_report, hid_report, sizeof(hid_report));
}

static bool SetHidKeyState(uint8_t keycode, bool pressed)
{
    if(keycode > 0xE7)
        return false;

    if(keycode >= 0xE0 && keycode <= 0xE7)
    {
        const uint8_t bit_mask = static_cast<uint8_t>(1U << (keycode - 0xE0));
        if(pressed)
            hid_report[0] |= bit_mask;
        else
            hid_report[0] &= static_cast<uint8_t>(~bit_mask);
        return true;
    }

    const uint8_t byte_index = static_cast<uint8_t>(keycode >> 3);
    if(byte_index >= (kNkroReportBytes - kNkroBitmapOffset))
        return false;

    const uint8_t bit_mask = static_cast<uint8_t>(1U << (keycode & 0x07U));
    uint8_t& byte = hid_report[kNkroBitmapOffset + byte_index];
    if(pressed)
        byte |= bit_mask;
    else
        byte &= static_cast<uint8_t>(~bit_mask);
    return true;
}

static bool SendHidReport()
{
    if(hid_class_id == kNoClass)
        return false;

    if(!HidReportChanged())
        return false;

    if(USBD_HID_SendReport(&hUsbDeviceHS, hid_report, sizeof(hid_report), hid_class_id) != USBD_OK)
        return false;

    SaveHidReport();
    return true;
}

static inline void ClearAllKeys()
{
    std::memset(hid_report, 0, sizeof(hid_report));
    SendHidReport();
}

static bool KeyOn(uint8_t keycode)
{
    const bool ok = SetHidKeyState(keycode, true);
    if(ok)
        SendHidReport();
    return ok;
}

static bool KeyOff(uint8_t keycode)
{
    const bool ok = SetHidKeyState(keycode, false);
    if(ok)
        SendHidReport();
    return ok;
}

static inline uint8_t CharToKeycode(char c)
{
    if(c >= 'a' && c <= 'z')
        return static_cast<uint8_t>(0x04 + (c - 'a'));

    if(c >= 'A' && c <= 'Z')
        return static_cast<uint8_t>(0x04 + (c - 'A'));

    if(c >= '1' && c <= '9')
        return static_cast<uint8_t>(0x1E + (c - '1'));

    switch(c)
    {
        case '0': return 0x27;
        case '\n': return 0x28;
        case '\r': return 0x28;
        case 0x1B: return 0x29;
        case '\b': return 0x2A;
        case '\t': return 0x2B;
        case ' ': return 0x2C;
        case '-': return 0x2D;
        case '=': return 0x2E;
        case '[': return 0x2F;
        case ']': return 0x30;
        case '\\': return 0x31;
        case ';': return 0x33;
        case '\'': return 0x34;
        case '`': return 0x35;
        case ',': return 0x36;
        case '.': return 0x37;
        case '/': return 0x38;
        default: return 0x00;
    }
}

static bool SetHidKeyA(bool pressed)
{
    return pressed ? KeyOn(kHidKeyA) : KeyOff(kHidKeyA);
}
#else
static bool SetHidKeyState(uint8_t, bool) { return false; }
static bool SendHidReport() { return false; }
static inline void ClearAllKeys() {}
static bool KeyOn(uint8_t) { return false; }
static bool KeyOff(uint8_t) { return false; }
static inline uint8_t CharToKeycode(char) { return 0U; }
static bool SetHidKeyA(bool) { return false; }
#endif

#if USBD_CMPSIT_ACTIVATE_MIDI
static void QueueMidiPacket(uint8_t cin, uint8_t status, uint8_t data1, uint8_t data2)
{
    midi_pending_packet[0] = cin;
    midi_pending_packet[1] = status;
    midi_pending_packet[2] = data1;
    midi_pending_packet[3] = data2;
    midi_tx_pending = true;
}

static bool SendMidiPacket(uint8_t cin, uint8_t status, uint8_t data1, uint8_t data2)
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

static int8_t MidiInit()
{
    midi_class_id = static_cast<uint8_t>(hUsbDeviceHS.classId);
    if(midi_class_id == kNoClass)
        return USBD_FAIL;
    return USBD_MIDI_SetRxBuffer(&hUsbDeviceHS, midi_rx_buffer, midi_class_id);
}

static int8_t MidiDeInit()
{
    return USBD_OK;
}

static int8_t MidiReceive(uint8_t* buf, uint32_t* len)
{
    if(buf == nullptr || len == nullptr)
        return USBD_FAIL;

    for(uint32_t offset = 0; offset + MIDI_USB_EVENT_PACKET_SIZE <= *len; offset += MIDI_USB_EVENT_PACKET_SIZE)
    {
        const uint8_t cin = buf[offset] & 0x0F;
        const uint8_t status = buf[offset + 1];
        const uint8_t data1 = buf[offset + 2];
        const uint8_t data2 = buf[offset + 3];

        if((cin == 0x09U) && ((status & 0xF0U) == 0x90U) && data2 != 0U)
        {
            QueueMidiPacket(0x09U, 0x90U, static_cast<uint8_t>(data1 + 1U), data2);
        }
        else if((cin == 0x08U) || (((status & 0xF0U) == 0x80U) || (((status & 0xF0U) == 0x90U) && data2 == 0U)))
        {
            QueueMidiPacket(0x08U, 0x80U, static_cast<uint8_t>(data1 + 1U), 0U);
        }
    }

    return USBD_OK;
}

static int8_t MidiTransmitComplete(uint8_t* buf, uint32_t* len, uint8_t epnum)
{
    (void)buf;
    (void)len;
    (void)epnum;
    return USBD_OK;
}
#endif

static void Process()
{
#if USBD_CMPSIT_ACTIVATE_MIDI
    if(midi_tx_pending
       && SendMidiPacket(midi_pending_packet[0],
                         midi_pending_packet[1],
                         midi_pending_packet[2],
                         midi_pending_packet[3]))
    {
        midi_tx_pending = false;
    }
#endif
}

} // namespace UsbComp

#if USBD_CMPSIT_ACTIVATE_AUDIO
extern "C" uint32_t AudioFifo_GetWritePtrLast(void)
{
    return UsbComp::audio_fifo_write_ptr_last;
}

extern "C" float AudioFifo_GetLeft(uint32_t index)
{
    return UsbComp::audio_fifo_l[index & UsbComp::kAudioRingMask];
}

extern "C" float AudioFifo_GetRight(uint32_t index)
{
    return UsbComp::audio_fifo_r[index & UsbComp::kAudioRingMask];
}

extern "C" uint32_t AudioFifo_GetRingSize(void)
{
    return UsbComp::kAudioRingSize;
}
#endif
