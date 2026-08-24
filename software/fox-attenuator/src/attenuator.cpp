#include "attenuator.hpp"

void Attenuator::Apply(int32_t value)
{
    if (bypass_ && value < 0) {
        bypass_ = false;
        return;
    }
    if (not bypass_ && value > 0 && attenuation_ == kMaxAttenuation) {
        bypass_ = true;
        return;
    }
    const int currentAttenuation = int32_t(attenuation_) + value;
    if (currentAttenuation >= kMaxAttenuation) {
        attenuation_ = kMaxAttenuation;
        return;
    }
    if (currentAttenuation <= kMinAttenuation) {
        attenuation_ = kMinAttenuation;
        return;
    }
    attenuation_ += value;
}

uint8_t Attenuator::GetAttenuatorBits(std::size_t attenuatorIdx)
{
    // lsb step must be applied only on one attenuator
    if (attenuatorIdx == 0) {
        const uint8_t lsbVal = (attenuation_ & 0b1);
        return (((attenuation_ >> 1) + lsbVal) & kMaxAttenuationPerAttenuator);
    }
    return (attenuation_ >> 1) & kMaxAttenuationPerAttenuator;
}
