#pragma once
#include "event.hpp"
#include <cstdint>
#include <zephyr/kernel.h>

class LogicManager
{
  public:
    explicit LogicManager(msgbus::Bus& bus);

    void Handle(const msgbus::events::EncoderRotated&) noexcept;
    void Handle(const msgbus::events::EncoderPressEvent&) noexcept;
    void Handle(const msgbus::events::ChargerEvent&) noexcept;
    void Handle(const msgbus::events::AttenuationInfo&) noexcept;

    using EventVariant = msgbus::SystemEvents::EventVariant;
    void EnqueueEvent(const EventVariant& event) noexcept;
    void Start();

  private:
    void ProcessEvent(const EventVariant& event) noexcept;
    void Run();

    msgbus::Bus& msgBus_;
    msgbus::SystemEvents::SubscriberAdapter<LogicManager, msgbus::events::EncoderRotated> subEncoderRotated_{*this};
    msgbus::SystemEvents::SubscriberAdapter<LogicManager, msgbus::events::EncoderPressEvent> subEncoderPressEvent_{
        *this};
    msgbus::SystemEvents::SubscriberAdapter<LogicManager, msgbus::events::ChargerEvent> subChargerEvent_{*this};
    msgbus::SystemEvents::SubscriberAdapter<LogicManager, msgbus::events::AttenuationInfo> subAttenuationInfo_{*this};

    k_thread threadData_{};
    k_msgq msgQueue_{};
    static constexpr size_t kQueueDepth = 16;
    alignas(EventVariant) std::array<char, sizeof(EventVariant) * kQueueDepth> msgBuffer_{};
    static void ThreadEntry(void* p1, void* p2, void* p3) noexcept
    {
        static_cast<LogicManager*>(p1)->Run();
    }
};
