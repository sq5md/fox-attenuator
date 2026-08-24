#pragma once

#include "attenuator.hpp"
#include "event.hpp"
#include "noncopyable.hpp"
#include "rgbLed.hpp"
#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>

class HardwareManager : public NonCopyable
{
    struct Key
    {};

  public:
    static HardwareManager* Create(msgbus::Bus& bus);
    explicit HardwareManager(Key, msgbus::Bus&);

    void Handle(const msgbus::events::LedMatrixSet&) noexcept;
    void Handle(const msgbus::events::PowerOff&) noexcept;
    void Handle(const msgbus::events::RgbLedSet&) noexcept;
    void Handle(const msgbus::events::ApplyAttenuationChange&) noexcept;
    using EventVariant = msgbus::SystemEvents::EventVariant;
    void EnqueueEvent(const EventVariant& event) noexcept;

  private:
    bool SetupHardware() noexcept;
    int SetupUsb();
    void ProcessEvent(const EventVariant& event) noexcept;
    void Run();

    void RfSwitchSetPath(bool antennaBypass);
    void SetAttenuator(uint8_t value1, uint8_t value2);
    void SetPinPowerOff();
    static void ThreadEntry(void* p1, void* p2, void* p3) noexcept
    {
        static_cast<HardwareManager*>(p1)->Run();
    }

    Attenuator attenuator_{};
    k_thread threadData_{};
    k_msgq msgQueue_{};
    static constexpr size_t kQueueDepth = 16;
    alignas(EventVariant) std::array<char, sizeof(EventVariant) * kQueueDepth> msgBuffer_{};
    msgbus::SystemEvents::SubscriberAdapter<HardwareManager, msgbus::events::LedMatrixSet> subLedMatrixSet_;
    msgbus::SystemEvents::SubscriberAdapter<HardwareManager, msgbus::events::PowerOff> subPowerHoldSet_;
    msgbus::SystemEvents::SubscriberAdapter<HardwareManager, msgbus::events::RgbLedSet> subRgbLedSet_;
    msgbus::SystemEvents::SubscriberAdapter<HardwareManager, msgbus::events::ApplyAttenuationChange>
        subApplyAttenuationChange_;
    msgbus::Bus& msgBus_;
    RgbLed rgbLed_;
    bool initialized_{false};
};
