#pragma once
#include <stdint.h>
#include "hid/ctrl.h"

namespace daisy
{
/** @addtogroup controls
    @{
*/

/** Simple parameter mapping tool that takes a 0-1 input from an AnalogControl and adds the following
 *  configurable post-processing:
 *  - Map to a specific curve and output range
 *  - Enables "virtaul parameters" with configurable update behavior
 *      - Selectively enable/disable parameters mapped to the same AnalogControl (e.g. while holding a "shift" button)
 *      - Configure how controls respond to raw input after becoming enabled again (via UpdateBehavior)
 */
class Parameter
{
  public:
    /** Curves are applied to the output signal */
    enum Curve
    {
        LINEAR,      /**< Linear curve */
        EXPONENTIAL, /**< Exponential curve */
        LOGARITHMIC, /**<  Logarithmic curve */
        CUBE,        /**< Cubic curve */
        LAST,        /**< Final enum element. */
    };

    enum UpdateBehavior
    {
        ALWAYS,    /**< Always update value while enabled */
        ON_CHANGE, /**< Update value only if changed after becoming enabled */
        PICKUP, /**< Update value only if it reaches previous value after becoming enabled */
        UPDATE_BEHAVIOR_LAST, /**< Final enum element */
    };

    /** Constructor */
    Parameter() {}
    /** Destructor */
    ~Parameter() {}

    /** initialize a parameter using an hid_ctrl object.
    \param input object containing the direct link to a hardware control source.
    \param min bottom of range. (when input is 0.0)
    \param max top of range (when input is 1.0)
    \param curve the scaling curve for the input->output transformation.
    \param update_behavior controls how previously disabled parameters update their value after becoming enabled
    */
    void Init(AnalogControl  input,
              float          min,
              float          max,
              Curve          curve,
              UpdateBehavior update_behavior = ALWAYS);

    /** Processes the input signal, this should be called at the samplerate of the AnalogControl passed in.
    \return  a float with the specified transformation applied.
    */
    float Process();

    /** Set whether the parameter is enabled. If disabled, it will stop
     *  updating its internal value on Process() */
    void SetEnabled(bool enabled);

    /** 
    \return the current value from the parameter without processing another sample.
    this is useful if you need to use the value multiple times, and don't store

    the output of process in a local variable.
    */
    inline float Value() { return out_val_; }

  private:
    AnalogControl  in_;
    float          pmin_, pmax_;
    float          lmin_, lmax_; // for log range
    float          val_, out_val_;
    float          raw_val_, d_raw_val_;
    bool           enabled_, tracking_;
    Curve          pcurve_;
    UpdateBehavior update_behavior_;
};
/** @} */
} // namespace daisy
