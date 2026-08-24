#pragma once

#include "noncopyable.hpp"

#include <array>
#include <cstdint>
#include <variant>

namespace msgbus
{
    template <typename T, typename Event>
    concept HandlerFor = requires(T& handler, const Event& event) {
        { handler.Handle(event) } noexcept;
    };

    template <typename... Events> class EventBus
    {
      public:
        using EventVariant = std::variant<Events...>;

        class ISubscriber
        {
          public:
            constexpr virtual ~ISubscriber()                         = default;
            virtual void OnEvent(const EventVariant& event) noexcept = 0;
        };

        template <typename Target, typename Event> class SubscriberAdapter final : public ISubscriber
        {
          public:
            constexpr explicit SubscriberAdapter(Target& instance) noexcept
                requires HandlerFor<Target, Event>
                : instance_{instance}
            {}

            void OnEvent(const EventVariant& event) noexcept override
            {
                if (std::holds_alternative<Event>(event)) {
                    instance_.EnqueueEvent(event);
                }
            }

          private:
            Target& instance_;
        };

        template <std::size_t MaxSubscribers> class Dispatcher : public NonCopyable
        {
          public:
            constexpr bool Subscribe(ISubscriber& subscriber) noexcept
            {
                if (count_ >= MaxSubscribers) {
                    return false;
                }
                subscribers_[count_++] = &subscriber;
                return true;
            }

            void Publish(const EventVariant& event) noexcept
            {
                for (std::size_t i = 0; i < count_; ++i) {
                    subscribers_[i]->OnEvent(event);
                }
            }

            template <typename E>
                requires(std::same_as<E, Events> || ...)
            void Publish(E&& ev) noexcept
            {
                Publish(EventVariant{std::forward<E>(ev)});
            }

          private:
            std::array<ISubscriber*, MaxSubscribers> subscribers_{};
            std::size_t count_{0};
        };
    };

} // namespace msgbus