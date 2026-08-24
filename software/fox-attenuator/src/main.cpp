#include "event.hpp"
#include "hardwareManager.hpp"
#include "inputListener.hpp"
#include "logicManager.hpp"
#include "watchdog.hpp"
#include <zephyr/kernel.h>

int main()
{
    auto wdt = Watchdog::Get();
    if (not wdt) {
        return -ENODEV;
    }

    wdt->Feed();

    msgbus::Bus eventBus;
    InputListener inputListener(eventBus);
    HardwareManager::Create(eventBus);
    LogicManager appLogic{eventBus};
    k_msleep(10);
    appLogic.Start();
    
    while (true) {
        wdt->Feed();
        k_msleep(1000);
    }

    return 0;
}
