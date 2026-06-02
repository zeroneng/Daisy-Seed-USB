#include "daisy_seed.h"

using namespace daisy;

DaisySeed      hw;
MidiUsbHandler midi;

static bool led_state   = false;

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetLed(false);

    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);

    uint32_t last_tx = 0;
    bool     note_on = false;

    while(1)
    {
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

        if(System::GetNow() - last_tx > 1000)
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
