#include "midi_util.h"
#include <string.h>
#include <algorithm>

using namespace daisy;

MidiTxMessage::MidiTxMessage()
{
    size = 0;
    std::fill(data, data + kMaxDataSize, 0);
}

MidiTxMessage MidiTxMessage::NoteOn(uint8_t ch, uint8_t nn, uint8_t vel)
{
    MidiTxMessage msg;
    msg.size = 3;
    // status byte
    msg.data[0] = 0x90 | (ch & 0x0F);
    // note num
    msg.data[1] = nn & 0x7F;
    // velocity
    msg.data[2] = vel & 0x7F;
    return msg;
}

MidiTxMessage MidiTxMessage::NoteOff(uint8_t ch, uint8_t nn, uint8_t vel)
{
    MidiTxMessage msg;
    msg.size = 3;
    // status byte
    msg.data[0] = 0x80 | (ch & 0x0F);
    // note num
    msg.data[1] = nn & 0x7F;
    // velocity
    msg.data[2] = vel & 0x7F;
    return msg;
}

MidiTxMessage MidiTxMessage::PitchBend(uint8_t ch, int16_t bend)
{
    MidiTxMessage msg;
    msg.size = 3;
    bend += 8192;
    // status byte
    msg.data[0] = 0xE0 | (ch & 0x0F);
    // lsb
    msg.data[1] = bend & 0x7F;
    // msb
    msg.data[2] = (bend >> 7) & 0x7F;
    return msg;
}

MidiTxMessage MidiTxMessage::SystemRealtimeClock()
{
    MidiTxMessage msg;
    msg.data[0] = 0xf8;
    msg.size    = 1;
    return msg;
}
MidiTxMessage MidiTxMessage::SystemRealtimeStart()
{
    MidiTxMessage msg;
    msg.data[0] = 0xfa;
    msg.size    = 1;
    return msg;
}
MidiTxMessage MidiTxMessage::SystemRealtimeStop()
{
    MidiTxMessage msg;
    msg.data[0] = 0xfc;
    msg.size    = 1;
    return msg;
}
MidiTxMessage MidiTxMessage::SystemExclusive(const uint8_t* data, size_t size)
{
    if(size > SYSEX_BUFFER_LEN)
        return MidiTxMessage();

    // Sysex data + start byte + stop byte
    MidiTxMessage msg;
    msg.size = size + 2;
    // start byte
    msg.data[0] = 0xF0;
    // data
    memcpy(msg.data + 1, data, size);
    // stop byte
    msg.data[size + 1] = 0xF7;
    return msg;
}
