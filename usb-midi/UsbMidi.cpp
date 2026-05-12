#include "daisy_seed.h"
extern "C" {
#include "usbh_midi.h"
}

using namespace daisy;

DaisySeed      hw;
USBHostHandle  usb_host;
MidiUsbHandler midi;

static bool class_ready = false;
static bool led_state   = false;

static void OnUsbClassActive(void* userdata)
{
    (void)userdata;
    class_ready = true;
}

static void OnUsbDisconnect(void* userdata)
{
    (void)userdata;
    class_ready = false;
    led_state   = false;
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetLed(false);

    USBHostHandle::Config usbh_cfg;
    usbh_cfg.class_active_callback = OnUsbClassActive;
    usbh_cfg.disconnect_callback   = OnUsbDisconnect;
    usb_host.Init(usbh_cfg);
    usb_host.RegisterClass(USBH_MIDI_CLASS);

    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::HOST;
    midi.Init(midi_cfg);

    uint32_t last_tx = 0;
    bool     note_on = false;

    while(1)
    {
        usb_host.Process();
        midi.Listen();

        while(midi.HasEvents())
        {
            auto msg = midi.PopEvent();
            switch(msg.type)
            {
                case NoteOn:
                {
                    auto note = msg.AsNoteOn();
                    if(note.velocity != 0)
                    {
                        led_state = true;
                        hw.SetLed(true);
                    }
                }
                break;
                case NoteOff:
                    led_state = false;
                    hw.SetLed(false);
                    break;
                default: break;
            }
        }

        if(class_ready && System::GetNow() - last_tx > 1000)
        {
            last_tx = System::GetNow();
            if(note_on)
            {
                uint8_t noteoff[3] = {0x80, 60, 0};
                midi.SendMessage(noteoff, 3);
                note_on = false;
            }
            else
            {
                uint8_t noteon[3] = {0x90, 60, 100};
                midi.SendMessage(noteon, 3);
                note_on = true;
            }
        }
    }
}
