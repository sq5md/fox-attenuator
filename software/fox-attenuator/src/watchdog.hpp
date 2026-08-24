#pragma once
#include <zephyr/device.h>

class Watchdog
{
public:
    static Watchdog* Get();
    void Feed();
private:
    Watchdog();
    bool initialized_{false};
    const device * const wdtDev_;
    int wdtChannelId_;
};

