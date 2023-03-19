/*
TODO:
- Add Blend(), Scale(), etc.
- I'd also like to change the way the Color names are accessed.
*/
/**        I'd like for it to be easy and not `Color::PresetColor::Foo`
*/
/** - There's no way to change a color once its been created (without unintuitively reiniting it).
*/
#pragma once
#ifndef DSY_COLOR_H
#define DSY_COLOR_H
#include <stdint.h>


namespace daisy
{
/** @addtogroup utility
    @{
*/

/** Class for handling simple colors */
class Color
{
  public:
    Color();
    ~Color() = default;
    Color(float red, float green, float blue);
    Color(uint8_t red, uint8_t green, uint8_t blue);
    Color(uint32_t hex);

    /** Returns the 0-1 value for Red */
    inline float Red() const { return red_; }

    /** Returns the 0-1 value for Green */
    inline float Green() const { return green_; }

    /** Returns the 0-1 value for Blue */
    inline float Blue() const { return blue_; }

    inline uint8_t Red8() const { return red_ * 255; }
    inline uint8_t Green8() const { return green_ * 255; }
    inline uint8_t Blue8() const { return blue_ * 255; }

    inline uint32_t Hex() const
    {
      uint32_t hex = 0;
      hex |= Red8() << 16;
      hex |= Green8() << 8;
      hex |= Blue8();
      return hex;
    }

    /** Returns a scaled color by a float */
    Color operator*(const float scale) const
    {
        return Color(red_ * scale, green_ * scale, blue_ * scale);
    }

  private:
    float              red_, green_, blue_;
};
/** @} */
} // namespace daisy

#endif
