#include "hid/parameter.h"
#include <math.h>

using namespace daisy;

static inline bool IsCloseTo(float in, float target, float epsilon = 0.001f)
{
    return fabsf(in - target) <= epsilon;
}

void Parameter::Init(AnalogControl  input,
                     float          min,
                     float          max,
                     Curve          curve,
                     UpdateBehavior update_behavior)
{
    pmin_            = min;
    pmax_            = max;
    pcurve_          = curve;
    in_              = input;
    lmin_            = logf(min < 0.0000001f ? 0.0000001f : min);
    lmax_            = logf(max);
    val_             = 0.0f;
    out_val_         = 0.0f;
    raw_val_         = 0.0f;
    d_raw_val_       = 0.0f;
    enabled_         = true;
    tracking_        = true;
    update_behavior_ = update_behavior;
}

float Parameter::Process()
{
    float in = in_.Process();
    raw_val_ = in_.GetRawFloat();

    switch(pcurve_)
    {
        case LINEAR: val_ = (in * (pmax_ - pmin_)) + pmin_; break;
        case EXPONENTIAL: val_ = ((in * in) * (pmax_ - pmin_)) + pmin_; break;
        case LOGARITHMIC: val_ = expf((in * (lmax_ - lmin_)) + lmin_); break;
        case CUBE: val_ = ((in * (in * in)) * (pmax_ - pmin_)) + pmin_; break;
        default: break;
    }

    if(enabled_)
    {
        if(tracking_)
        {
            out_val_ = val_;
        }
        else
        {
            switch(update_behavior_)
            {
                case ON_CHANGE:
                    if(!IsCloseTo(raw_val_, d_raw_val_))
                    {
                        tracking_ = true;
                        out_val_  = val_;
                    }
                    break;
                case PICKUP:
                    if(IsCloseTo(raw_val_, d_raw_val_))
                    {
                        tracking_ = true;
                        out_val_  = val_;
                    }
                    break;
                default:
                    tracking_ = true;
                    out_val_  = val_;
                    break;
            }
        }
    }

    return out_val_;
}

void Parameter::SetEnabled(bool enabled)
{
    if(!enabled && enabled_)
    {
        tracking_ = false;
        if(update_behavior_ == PICKUP)
        {
            d_raw_val_ = raw_val_;
        }
    }
    else if(enabled && !enabled_ && update_behavior_ == ON_CHANGE)
    {
        d_raw_val_ = raw_val_;
    }
    enabled_ = enabled;
}