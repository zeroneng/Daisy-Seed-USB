#pragma once

// Header-only application bridge for usb-comp.
//
// Include this from exactly one C++ translation unit, normally Main.cpp.
// It defines the AudioFifo_* C symbols consumed by usbd_audio_if.c.

#include <stdint.h>

#ifndef USB_COMP_ENABLE_CDC
#ifdef USBD_CMPSIT_ACTIVATE_CDC
#define USB_COMP_ENABLE_CDC USBD_CMPSIT_ACTIVATE_CDC
#else
#define USB_COMP_ENABLE_CDC 1
#endif
#endif

#ifndef USB_COMP_ENABLE_HID
#ifdef USBD_CMPSIT_ACTIVATE_HID
#define USB_COMP_ENABLE_HID USBD_CMPSIT_ACTIVATE_HID
#else
#define USB_COMP_ENABLE_HID 1
#endif
#endif

#ifndef USB_COMP_ENABLE_AUDIO
#ifdef USBD_CMPSIT_ACTIVATE_AUDIO
#define USB_COMP_ENABLE_AUDIO USBD_CMPSIT_ACTIVATE_AUDIO
#else
#define USB_COMP_ENABLE_AUDIO 1
#endif
#endif

#ifndef USB_COMP_ENABLE_MIDI
#ifdef USBD_CMPSIT_ACTIVATE_MIDI
#define USB_COMP_ENABLE_MIDI USBD_CMPSIT_ACTIVATE_MIDI
#else
#define USB_COMP_ENABLE_MIDI 1
#endif
#endif

#ifndef USB_COMP_ENABLE_MSC
#ifdef USBD_CMPSIT_ACTIVATE_MSC
#define USB_COMP_ENABLE_MSC USBD_CMPSIT_ACTIVATE_MSC
#else
#define USB_COMP_ENABLE_MSC 0
#endif
#endif

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_def.h"

#if USB_COMP_ENABLE_CDC
#include "usb_comp_cdc_if.h"
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

namespace UsbComp
{
constexpr uint8_t kNoClass = 0xFFU;

#if USB_COMP_ENABLE_CDC
static uint8_t cdc_class_id = kNoClass;
static uint8_t cdc_ep_addr[] = {0x82U, 0x02U, 0x86U};
#endif

#if USB_COMP_ENABLE_HID
constexpr uint8_t kNkroReportBytes = 33;
constexpr uint8_t kNkroBitmapOffset = 1;
constexpr uint8_t kHidKeyA = 0x04;
static uint8_t hid_class_id = kNoClass;
static uint8_t hid_ep_addr[] = {0x84U};
static uint8_t hid_report[kNkroReportBytes] = {};
#endif

#if USB_COMP_ENABLE_AUDIO
static uint8_t audio_class_id = kNoClass;
static uint8_t audio_ep_addr[] = {AUDIO_OUT_EP, AUDIO_IN_EP};

#ifndef USB_COMP_AUDIO_CAPTURE_RING_SIZE
#define USB_COMP_AUDIO_CAPTURE_RING_SIZE 16384u
#endif

static_assert(USB_COMP_AUDIO_CAPTURE_RING_SIZE != 0u
                  && ((USB_COMP_AUDIO_CAPTURE_RING_SIZE
                       & (USB_COMP_AUDIO_CAPTURE_RING_SIZE - 1u))
                      == 0u),
              "USB_COMP_AUDIO_CAPTURE_RING_SIZE must be a power of two");

constexpr uint32_t kAudioRingSize = USB_COMP_AUDIO_CAPTURE_RING_SIZE;
constexpr uint32_t kAudioRingMask = kAudioRingSize - 1u;

static float audio_fifo_l[kAudioRingSize] USB_COMP_AUDIO_BUFFER_MEM = {};
static float audio_fifo_r[kAudioRingSize] USB_COMP_AUDIO_BUFFER_MEM = {};
static uint32_t audio_fifo_write_ptr = 0u;
static uint32_t audio_fifo_write_ptr_last = 0u;
#endif

#if USB_COMP_ENABLE_MIDI
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
#if USB_COMP_ENABLE_AUDIO
    audio_fifo_write_ptr = 0u;
    audio_fifo_write_ptr_last = 0u;
    for(uint32_t i = 0; i < kAudioRingSize; i++)
    {
        audio_fifo_l[i] = 0.0f;
        audio_fifo_r[i] = 0.0f;
    }
#endif

    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);

#if USB_COMP_ENABLE_HID
    USBD_RegisterClassComposite(&hUsbDeviceHS,
                                USBD_HID_CLASS,
                                CLASS_TYPE_HID,
                                hid_ep_addr);
    hid_class_id = static_cast<uint8_t>(
        USBD_CMPSIT_GetClassID(&hUsbDeviceHS, CLASS_TYPE_HID, 0));
#endif

#if USB_COMP_ENABLE_CDC
    USBD_RegisterClassComposite(&hUsbDeviceHS,
                                USBD_CDC_CLASS,
                                CLASS_TYPE_CDC,
                                cdc_ep_addr);
    cdc_class_id = static_cast<uint8_t>(
        USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_CDC, 0));
    USB_COMP_CDC_SetClassId(cdc_class_id);
    USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USB_COMP_CDC_Interface_fops_HS);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

#if USB_COMP_ENABLE_AUDIO
    USBD_RegisterClassComposite(&hUsbDeviceHS,
                                USBD_AUDIO_CLASS,
                                CLASS_TYPE_AUDIO,
                                audio_ep_addr);
    audio_class_id = static_cast<uint8_t>(
        USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_AUDIO, 0));
    USBD_AUDIO_RegisterInterface(&hUsbDeviceHS, &USBD_AUDIO_fops);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

#if USB_COMP_ENABLE_MIDI
    USBD_RegisterClassComposite(&hUsbDeviceHS,
                                USBD_MIDI_CLASS,
                                CLASS_TYPE_MIDI,
                                midi_ep_addr);
    midi_class_id = static_cast<uint8_t>(
        USBD_CMPSIT_SetClassID(&hUsbDeviceHS, CLASS_TYPE_MIDI, 0));
    USBD_MIDI_RegisterInterface(&hUsbDeviceHS, &midi_fops);
    hUsbDeviceHS.classId = hUsbDeviceHS.NumClasses;
#endif

    USBD_Start(&hUsbDeviceHS);
}

#if USB_COMP_ENABLE_AUDIO
static void PushCapture(float left, float right)
{
    audio_fifo_l[audio_fifo_write_ptr] = left;
    audio_fifo_r[audio_fifo_write_ptr] = right;
    audio_fifo_write_ptr = (audio_fifo_write_ptr + 1u) & kAudioRingMask;
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

#if USB_COMP_ENABLE_CDC
static bool SendCdc(const char* text)
{
    if(text == nullptr || cdc_class_id == kNoClass || !USB_COMP_CDC_IsConnected())
        return false;

    uint16_t len = 0;
    while(text[len] != '\0')
        len++;

    return CDC_Transmit_HS((uint8_t*)text, len) == USBD_OK;
}
#else
static bool SendCdc(const char*) { return false; }
#endif

#if USB_COMP_ENABLE_HID
static void SetHidKeyState(uint8_t keycode, bool pressed)
{
    if(keycode > 0xE7)
        return;

    if(keycode >= 0xE0 && keycode <= 0xE7)
    {
        const uint8_t bit_mask = static_cast<uint8_t>(1U << (keycode - 0xE0));
        if(pressed)
            hid_report[0] |= bit_mask;
        else
            hid_report[0] &= static_cast<uint8_t>(~bit_mask);
        return;
    }

    const uint8_t byte_index = static_cast<uint8_t>(keycode >> 3);
    if(byte_index >= (kNkroReportBytes - kNkroBitmapOffset))
        return;

    const uint8_t bit_mask = static_cast<uint8_t>(1U << (keycode & 0x07U));
    uint8_t& byte = hid_report[kNkroBitmapOffset + byte_index];
    if(pressed)
        byte |= bit_mask;
    else
        byte &= static_cast<uint8_t>(~bit_mask);
}

static bool SendHidReport()
{
    if(hid_class_id == kNoClass)
        return false;

    return USBD_HID_SendReport(&hUsbDeviceHS, hid_report, sizeof(hid_report), hid_class_id) == USBD_OK;
}

static bool SetHidKeyA(bool pressed)
{
    SetHidKeyState(kHidKeyA, pressed);
    return SendHidReport();
}
#else
static bool SetHidKeyA(bool) { return false; }
#endif

#if USB_COMP_ENABLE_MIDI
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
#if USB_COMP_ENABLE_MIDI
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

#if USB_COMP_ENABLE_AUDIO
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

extern "C" uint32_t AudioFifo_GetRingMask(void)
{
    return UsbComp::kAudioRingMask;
}
#endif
