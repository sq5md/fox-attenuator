#pragma once

#include "event.hpp"
#include <zephyr/input/input.h>

class InputListener
{
  public:
    InputListener(msgbus::Bus& eventBus);
    ~InputListener();

  private:
    static void InputCallbackWrapper(input_event* evt, void* user_data);
    static InputListener* instance_; // "user_data" not works dynamicly

    void InputCallback(input_event* evt);

    msgbus::Bus& eventBus_;
};
