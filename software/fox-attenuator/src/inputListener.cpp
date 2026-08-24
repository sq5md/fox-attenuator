#include "inputListener.hpp"
// #include "zephyr/logging/log.h"
// #include "zephyr/logging/log_ctrl.h"

// LOG_MODULE_REGISTER(inputListener, CONFIG_LOG_DEFAULT_LEVEL);

InputListener* InputListener::instance_{nullptr};

InputListener::InputListener(msgbus::Bus& eventBus) : eventBus_(eventBus)
{
    instance_ = this;
    INPUT_CALLBACK_DEFINE(nullptr, InputCallbackWrapper, nullptr);
}
InputListener::~InputListener()
{}
void InputListener::InputCallbackWrapper(input_event* evt, void* user_data)
{
    if (not instance_) {
        return;
    }
    instance_->InputCallback(evt);
}

void InputListener::InputCallback(input_event* evt)
{
    switch (evt->type) {
    case INPUT_EV_REL: {
        if (evt->code == INPUT_REL_X) {
            eventBus_.Publish(msgbus::events::EncoderRotated{evt->value});
        }
        break;
    }
    case INPUT_EV_KEY: {
        if (evt->code == INPUT_KEY_ENTER) {
            using PressType                 = msgbus::events::EncoderPressEvent::Type;
            constexpr int32_t kValuePressed = 1;
            const auto pressType            = evt->value == kValuePressed ? PressType::kPressed : PressType::kReleased;
            eventBus_.Publish(msgbus::events::EncoderPressEvent{pressType});
        }
        else if (evt->code == INPUT_KEY_0) {
            eventBus_.Publish(msgbus::events::ChargerEvent{evt->value != 0});
        }
        break;
    }
    }
}