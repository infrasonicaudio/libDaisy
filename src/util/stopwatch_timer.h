#pragma once

#include <sys/system.h>

namespace daisy
{
/// Dead simple stopwatch style soft-timer
/// Operates on Systick rounded to milliseconds
class StopwatchTimer
{
  public:
    StopwatchTimer()  = default;
    ~StopwatchTimer() = default;

    inline void Init() { Restart(); }

    inline bool HasPassedUs(uint32_t us)
    {
        const auto now = daisy::System::GetTick();
        return daisy::System::GetUsBetweenTicks(now, last_) >= us;
    }

    inline bool HasPassedMs(uint32_t ms)
    {
        return HasPassedUs(ms * 1000);
    }

    inline void Restart() { last_ = daisy::System::GetTick(); }

  private:
    uint32_t last_;
};

} // namespace infrasonic
