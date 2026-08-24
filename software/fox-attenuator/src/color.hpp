#pragma once

#include <cstdint>

struct Color
{
    const uint8_t red;
    const uint8_t green;
    const uint8_t blue;
};

namespace colors
{
    constexpr auto kRed    = Color{255, 0, 0};
    constexpr auto kGreen  = Color{0, 255, 0};
    constexpr auto kBlue   = Color{0, 0, 255};
    constexpr auto kViolet = Color{255, 0, 255};
    constexpr auto kBlack  = Color{0, 0, 0};
} // namespace colors