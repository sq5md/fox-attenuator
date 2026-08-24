#include "watchdog.hpp"
#include <zephyr/drivers/watchdog.h>

#include <optional>
#include <zephyr/device.h>

#define WDT_NODE DT_NODELABEL(wdt0)

namespace
{
    std::optional<Watchdog> watchdogObj;
}

Watchdog* Watchdog::Get()
{
    watchdogObj.emplace(Watchdog{});
    if (watchdogObj->initialized_) {
        return &watchdogObj.value();
    }
    return nullptr;
}

Watchdog::Watchdog() : wdtDev_(DEVICE_DT_GET(WDT_NODE))
{
    if (!device_is_ready(wdtDev_)) {
        initialized_ = false;
    }

    wdt_timeout_cfg wdtConfig = {};

    wdtConfig.window.min = 0U;
    wdtConfig.window.max = 3000U;
    wdtConfig.flags      = WDT_FLAG_RESET_SOC;
    wdtConfig.callback   = nullptr;

    wdtChannelId_ = wdt_install_timeout(wdtDev_, &wdtConfig);
    if (wdtChannelId_ < 0) {
        return;
    }

    if (wdt_setup(wdtDev_, WDT_OPT_PAUSE_HALTED_BY_DBG) < 0) {
        return;
    }
    initialized_ = true;
}

void Watchdog::Feed()
{
    wdt_feed(wdtDev_, wdtChannelId_);
}