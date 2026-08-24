#include "rgbLed.hpp"

#include "color.hpp"

#include <zephyr/drivers/led_strip.h>

void RgbLed::SetColor(Color color)
{
    led_rgb ledColor{};
    ledColor.r = color.green;
    ledColor.g = color.red;
    ledColor.b = color.blue;
    led_strip_update_rgb(ledStrip_, &ledColor, 1);
}
bool RgbLed::Ready() const
{
    return device_is_ready(ledStrip_);
}
