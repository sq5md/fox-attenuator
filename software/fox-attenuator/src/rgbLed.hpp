#pragma once
#include "color.hpp"
#include "zephyr/device.h"

class RgbLed
{
  public:
    RgbLed(const device* const ledStrip) : ledStrip_(ledStrip)
    {}
    void SetColor(Color color);
    [[nodiscard]] bool Ready() const;

  private:
    const device* const ledStrip_;
};
