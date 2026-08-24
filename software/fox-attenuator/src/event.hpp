#pragma once
#include "color.hpp"
#include "eventBusImpl.hpp"
#include <cstdint>

namespace msgbus
{
    namespace events
    {
        struct EncoderPressEvent
        {
            enum class Type
            {
                kPressed,
                kReleased,
            };

            const Type eventType;
        };
        struct EncoderRotated
        {
            const int32_t value;
        };
        struct LedMatrixSet
        {
            const uint8_t value;
        };
        struct RgbLedSet
        {
            const Color color;
        };
        struct PowerOff
        {};

        struct ApplyAttenuationChange
        {
            const int32_t value;
        };

        struct AttenuationInfo
        {
            const uint32_t attenuationHalfDb;
            const uint32_t maxAttenuationHalfDb;
            const bool bypass;
        };
        struct ChargerEvent
        {
            const bool isCharged;
        };

    } // namespace events

    using SystemEvents = EventBus<std::monostate,
                                  events::EncoderRotated,
                                  events::EncoderPressEvent,
                                  events::LedMatrixSet,
                                  events::RgbLedSet,
                                  events::PowerOff,
                                  events::ApplyAttenuationChange,
                                  events::AttenuationInfo,
                                  events::ChargerEvent>;
    using Bus          = SystemEvents::Dispatcher<10>;
} // namespace msgbus