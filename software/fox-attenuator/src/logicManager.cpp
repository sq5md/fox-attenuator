#include "logicManager.hpp"

namespace
{
    K_THREAD_STACK_DEFINE(gStack, 2048);
} // namespace

LogicManager::LogicManager(msgbus::Bus& bus) : msgBus_(bus)
{
    msgBus_.Subscribe(subEncoderRotated_);
    msgBus_.Subscribe(subEncoderPressEvent_);
    msgBus_.Subscribe(subChargerEvent_);
    msgBus_.Subscribe(subAttenuationInfo_);

    k_msgq_init(&msgQueue_, msgBuffer_.data(), sizeof(EventVariant), kQueueDepth);
    k_thread_create(&threadData_,
                    gStack,
                    K_THREAD_STACK_SIZEOF(gStack),
                    ThreadEntry,
                    this,
                    nullptr,
                    nullptr,
                    K_PRIO_COOP(7),
                    0,
                    K_NO_WAIT);
}

void LogicManager::Handle(const msgbus::events::EncoderRotated& er) noexcept
{
    msgBus_.Publish(msgbus::events::ApplyAttenuationChange{er.value});
}

void LogicManager::Handle(const msgbus::events::AttenuationInfo& er) noexcept
{
    constexpr uint8_t kMaxColorValue      = 0xFFU;
    constexpr uint8_t kShadingCoefficient = 5;
    if (er.bypass) {
        msgBus_.Publish(msgbus::events::RgbLedSet{colors::kBlue});
        msgBus_.Publish(msgbus::events::LedMatrixSet{0});
    }
    else {
        constexpr auto kLedCnt = 5;
        const uint8_t ledsOn =
            (er.attenuationHalfDb * kLedCnt + (er.maxAttenuationHalfDb / 2)) / er.maxAttenuationHalfDb;
        const uint8_t ledMatrix = (1U << ledsOn) - 1;
        const uint8_t r         = ((kMaxColorValue * er.attenuationHalfDb) / er.maxAttenuationHalfDb);
        const uint8_t g         = (kMaxColorValue - r);
        if (er.attenuationHalfDb == 0) {
            msgBus_.Publish(msgbus::events::RgbLedSet{colors::kGreen});
        }
        else if (er.attenuationHalfDb == er.maxAttenuationHalfDb) {
            msgBus_.Publish(msgbus::events::RgbLedSet{colors::kRed});
        }
        else {
            const auto shadedRed      = uint8_t(r / kShadingCoefficient);
            const auto shadedGreen    = uint8_t(g / kShadingCoefficient);
            constexpr auto shadedBlue = 0;

            msgBus_.Publish(msgbus::events::RgbLedSet{Color{shadedRed, shadedGreen, shadedBlue}});
        }
        msgBus_.Publish(msgbus::events::LedMatrixSet{.value = ledMatrix});
    }
}

void LogicManager::Handle(const msgbus::events::EncoderPressEvent& epe) noexcept
{
    if (epe.eventType == msgbus::events::EncoderPressEvent::Type::kPressed) {
        msgBus_.Publish(msgbus::events::PowerOff{});
    }
}

void LogicManager::Handle(const msgbus::events::ChargerEvent& ce) noexcept
{
    if (ce.isCharged) {
        msgBus_.Publish(msgbus::events::RgbLedSet{colors::kViolet});
    }
}

void LogicManager::EnqueueEvent(const EventVariant& event) noexcept
{
    k_msgq_put(&msgQueue_, &event, K_NO_WAIT);
}

void LogicManager::Start()
{
    // simulate encoder rotation to setup all components
    msgBus_.Publish(msgbus::events::EncoderRotated{-1});
}

void LogicManager::Run()
{
    msgbus::SystemEvents::EventVariant event;
    while (true) {
        if (k_msgq_get(&msgQueue_, &event, K_HOURS(1)) == 0) {
            ProcessEvent(event);
        }
        else {
            msgBus_.Publish(msgbus::events::PowerOff{});
        }
    }
}

void LogicManager::ProcessEvent(const msgbus::SystemEvents::EventVariant& event) noexcept
{
    std::visit(
        [this](const auto& concrete_event) {
            if constexpr (requires { Handle(concrete_event); }) {
                Handle(concrete_event);
            }
        },
        event);
}