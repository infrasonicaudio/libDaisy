#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hid/MidiEvent.h"

namespace daisy
{
struct MidiTxMessage
{
    static constexpr size_t kMaxDataSize = SYSEX_BUFFER_LEN + 2;

    uint8_t data[kMaxDataSize];
    size_t  size;

    MidiTxMessage();
    MidiTxMessage(const uint8_t* data, size_t size);
    ~MidiTxMessage() = default;

    static MidiTxMessage NoteOn(uint8_t ch, uint8_t nn, uint8_t vel);
    static MidiTxMessage NoteOff(uint8_t ch, uint8_t nn, uint8_t vel);
    static MidiTxMessage ControlChange(uint8_t ch, uint8_t ctl, uint8_t val);
    static MidiTxMessage PitchBend(uint8_t ch, int16_t bend);
    static MidiTxMessage SystemRealtimeClock();
    static MidiTxMessage SystemRealtimeStart();
    static MidiTxMessage SystemRealtimeStop();
    static MidiTxMessage SystemExclusive(const uint8_t* data, size_t size);
};

template <size_t kSize>
class MidiTxBuffer
{
  public:
    MidiTxBuffer(){};
    ~MidiTxBuffer(){};

    void Init(bool running_status_enabled)
    {
        size_        = 0;
        last_status_ = 0;
        rs_enabled_  = running_status_enabled;
    }

    bool IsWriteable(size_t added) const { return size_ + added < kSize; }

    bool IsEmpty() const { return size_ == 0; }

    // Trims bytes out for running status (assumes first byte is status)
    bool WriteMessage(const uint8_t* data, size_t size)
    {
        if(!IsWriteable(size))
            return false;

        const bool is_valid_for_running_status
            = rs_enabled_ && size > 1 && data[0] != 0xf0;

        if(is_valid_for_running_status && data[0] == last_status_)
        {
            memcpy(&buf_[size_], data + 1, size - 1);
            size_ += size - 1;
        }
        else
        {
            memcpy(&buf_[size_], data, size);
            size_ += size;
            last_status_ = is_valid_for_running_status ? data[0] : 0;
        }
        return true;
    }

    const uint8_t* GetData() const { return buf_; }
    size_t         GetSize() const { return size_; }

    void Consume()
    {
        size_        = 0;
        last_status_ = 0;
    }

  private:
    size_t  size_;
    uint8_t buf_[kSize];
    uint8_t last_status_;
    bool    rs_enabled_;
};
} // namespace daisy
