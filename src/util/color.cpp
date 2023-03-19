#include "util/color.h"
#include <algorithm>

using namespace daisy;

#define RED 0
#define GREEN 1
#define BLUE 2

namespace daisy
{
template <typename T>
T clamp(T in, T low, T high)
{
    return (in < low) ? low : (high < in) ? high : in;
}
} // namespace daisy

Color::Color() : red_(1.0f), green_(1.0f), blue_(1.0f) {}

Color::Color(float red, float green, float blue)
: red_(clamp(red, 0.0f, 1.0f)),
  green_(clamp(green, 0.0f, 1.0f)),
  blue_(clamp(blue, 0.0f, 1.0f))
{
}

Color::Color(uint8_t red, uint8_t green, uint8_t blue)
: red_(red / 255.0f), green_(green / 255.0f), blue_(blue / 255.0f)
{
}

Color::Color(uint32_t hex)
: Color(uint8_t((hex >> 16) & 0x00FF),
        uint8_t((hex >> 8) & 0x00FF),
        uint8_t(hex & 0x00FF))
{
}
