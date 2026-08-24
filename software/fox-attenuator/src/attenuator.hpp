#pragma once

#include "event.hpp"
#include <cstdint>

class Attenuator
{
  public:
    Attenuator() = default;
    void Apply(int32_t value);
    uint8_t GetAttenuatorBits(std::size_t attenuatorIdx);
    [[nodiscard]] bool GetBypass() const
    {
        return bypass_;
    }
    [[nodiscard]] uint8_t GetAttenuation() const
    {
        return attenuation_;
    }
    [[nodiscard]] static uint8_t GetMaxAttenuation()
    {
        return kMaxAttenuation;
    }

  private:
    bool bypass_{false};
    uint8_t attenuation_{0};

    static constexpr uint8_t kMinAttenuation              = 0UL;
    static constexpr uint8_t kMaxAttenuationPerAttenuator = 0b0011'1111UL;
    static constexpr uint8_t kMaxAttenuation              = 0b0111'1110UL;
};
