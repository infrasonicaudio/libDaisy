// DualMIDIRx
// Example of
#include "daisy_seed.h"

using namespace daisy;

static DaisySeed       hw;
static MidiUartHandler midi_uart1;
static MidiUartHandler midi_uart4;

int main(void)
{
    hw.Init(true);

    // This one we initialize with buffer defaults.
    // It uses the default DMA stream and circular buffer.
    MidiUartHandler::Config cfg1;
    cfg1.transport_config.periph = UartHandler::Config::Peripheral::USART_1;
    cfg1.transport_config.rx     = seed::D14;
    cfg1.transport_config.tx     = seed::D13;
    midi_uart1.Init(cfg1);

    // This one we initialize with alternate DMA so it uses a separate DMA
    // stream and circular buffer. UART4 is used as the peripheral in this example
    // but the peripheral and pins can be set for any other unused UART instead.
    MidiUartHandler::Config cfg4;
    cfg4.transport_config.WithAltDMA();
    cfg4.transport_config.periph    = UartHandler::Config::Peripheral::UART_4;
    cfg4.transport_config.rx        = seed::D11;
    cfg4.transport_config.tx        = seed::D12;
    midi_uart4.Init(cfg4);

    while(1)
    {
        midi_uart1.Listen();
        midi_uart4.Listen();

        while(midi_uart1.HasEvents())
        {
            // Handle UART1 MIDI events here
            const auto event = midi_uart1.PopEvent();
            // Quick LED flash to show event activity
            hw.SetLed(true);
            System::Delay(1);
            hw.SetLed(false);
            // ... do something with event
        }

        while(midi_uart4.HasEvents())
        {
            // Handle UART4 MIDI events here
            const auto event = midi_uart4.PopEvent();
            // Quick LED flash to show event activity
            hw.SetLed(true);
            System::Delay(1);
            hw.SetLed(false);
            // ... do something with event
        }
    }
}
