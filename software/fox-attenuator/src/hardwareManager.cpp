#include "hardwareManager.hpp"

#include "event.hpp"

#include <algorithm>
#include <optional>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usbd.h>

namespace
{
    K_THREAD_STACK_DEFINE(hardwareManagerStack, 2048);
    const gpio_dt_spec led0    = GPIO_DT_SPEC_GET(DT_NODELABEL(led9), gpios);
    const gpio_dt_spec led1    = GPIO_DT_SPEC_GET(DT_NODELABEL(led10), gpios);
    const gpio_dt_spec led2    = GPIO_DT_SPEC_GET(DT_NODELABEL(led11), gpios);
    const gpio_dt_spec led3    = GPIO_DT_SPEC_GET(DT_NODELABEL(led12), gpios);
    const gpio_dt_spec led4    = GPIO_DT_SPEC_GET(DT_NODELABEL(led13), gpios);
    const gpio_dt_spec pwrOff  = GPIO_DT_SPEC_GET(DT_NODELABEL(pwr_off_gpio), gpios);
    const gpio_dt_spec btnGpio = GPIO_DT_SPEC_GET(DT_NODELABEL(btn_gpio), gpios);
    const gpio_dt_spec clk1    = GPIO_DT_SPEC_GET(DT_ALIAS(bda_clk1), gpios);
    const gpio_dt_spec serin1  = GPIO_DT_SPEC_GET(DT_ALIAS(bda_serin1), gpios);
    const gpio_dt_spec clk2    = GPIO_DT_SPEC_GET(DT_ALIAS(bda_clk2), gpios);
    const gpio_dt_spec serin2  = GPIO_DT_SPEC_GET(DT_ALIAS(bda_serin2), gpios);
    const gpio_dt_spec le      = GPIO_DT_SPEC_GET(DT_ALIAS(bda_le), gpios);
    const gpio_dt_spec rfCtrl  = GPIO_DT_SPEC_GET(DT_ALIAS(rf_sw_ctrl), gpios);
    const gpio_dt_spec rfCtrlN = GPIO_DT_SPEC_GET(DT_ALIAS(rf_sw_ctrl_n), gpios);
    const device* ledStrip     = DEVICE_DT_GET(DT_ALIAS(led_strip));

    USBD_DEVICE_DEFINE(usbdevice, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2e8a, 0x000a);

    USBD_DESC_LANG_DEFINE(usbdlang);
    USBD_DESC_MANUFACTURER_DEFINE(usbdeviceManufacturer, "SP5YAM");
    USBD_DESC_PRODUCT_DEFINE(usbdeviceProduct, "Fox attenuator");

    // 500 mA declararion
    USBD_CONFIGURATION_DEFINE(usbdeviceConfig, USB_SCD_SELF_POWERED, 250, nullptr);

} // namespace

namespace
{
    std::optional<HardwareManager> hardwareManagerObj;
}

HardwareManager* HardwareManager::Create(msgbus::Bus& bus)
{
    if (not hardwareManagerObj) {
        hardwareManagerObj.emplace(Key{}, bus);
        if (hardwareManagerObj->initialized_) {
            return &hardwareManagerObj.value();
        }
        hardwareManagerObj = std::nullopt;
    }
    return nullptr;
}

bool isBitSet(uint8_t val, uint8_t bit)
{
    return (val & (1 << bit)) != 0;
}

void HardwareManager::Handle(const msgbus::events::LedMatrixSet& lm) noexcept
{
    const auto val = lm.value;
    gpio_pin_set_dt(&led0, isBitSet(val, 0) ? 1 : 0);
    gpio_pin_set_dt(&led1, isBitSet(val, 1) ? 1 : 0);
    gpio_pin_set_dt(&led2, isBitSet(val, 2) ? 1 : 0);
    gpio_pin_set_dt(&led3, isBitSet(val, 3) ? 1 : 0);
    gpio_pin_set_dt(&led4, isBitSet(val, 4) ? 1 : 0);
}
void HardwareManager::Handle(const msgbus::events::PowerOff& ph) noexcept
{
    SetPinPowerOff();
}
void HardwareManager::Handle(const msgbus::events::RgbLedSet& rgb) noexcept
{
    rgbLed_.SetColor(rgb.color);
}
void HardwareManager::Handle(const msgbus::events::ApplyAttenuationChange& aac) noexcept
{
    attenuator_.Apply(aac.value);
    const auto attenuator0Val = attenuator_.GetAttenuatorBits(0);
    const auto attenuator1Val = attenuator_.GetAttenuatorBits(1);
    SetAttenuator(attenuator0Val, attenuator1Val);

    if (attenuator_.GetBypass()) {
        RfSwitchSetPath(true);
    }
    else {
        RfSwitchSetPath(false);
    }

    msgBus_.Publish(msgbus::events::AttenuationInfo{.attenuationHalfDb    = attenuator_.GetAttenuation(),
                                                    .maxAttenuationHalfDb = Attenuator::GetMaxAttenuation(),
                                                    .bypass               = attenuator_.GetBypass()});
}
void HardwareManager::EnqueueEvent(const EventVariant& event) noexcept
{
    k_msgq_put(&msgQueue_, &event, K_NO_WAIT);
}

HardwareManager::HardwareManager(Key, msgbus::Bus& bus)
    : subLedMatrixSet_{*this}, subPowerHoldSet_{*this}, subRgbLedSet_{*this}, subApplyAttenuationChange_{*this},
      msgBus_(bus), rgbLed_(ledStrip)
{
    initialized_ = SetupHardware();
    if (not initialized_) {
        return;
    }

    bus.Subscribe(subLedMatrixSet_);
    bus.Subscribe(subPowerHoldSet_);
    bus.Subscribe(subRgbLedSet_);
    bus.Subscribe(subApplyAttenuationChange_);

    k_msgq_init(&msgQueue_, msgBuffer_.data(), sizeof(EventVariant), kQueueDepth);
    k_thread_create(&threadData_,
                    hardwareManagerStack,
                    K_THREAD_STACK_SIZEOF(hardwareManagerStack),
                    ThreadEntry,
                    this,
                    nullptr,
                    nullptr,
                    K_PRIO_COOP(7),
                    0,
                    K_NO_WAIT);
}

// helpers
namespace
{
    template <std::size_t N> bool IsReady(const std::array<const gpio_dt_spec*, N>& ary)
    {
        return std::ranges::all_of(
            ary.begin(), ary.end(), [](const gpio_dt_spec* item) { return gpio_is_ready_dt(item); });
    }

    template <std::size_t N> void SetInactive(const std::array<const gpio_dt_spec*, N>& ary)
    {
        std::ranges::for_each(ary.begin(), ary.end(), [](const gpio_dt_spec* item) {
            gpio_pin_configure_dt(item, GPIO_OUTPUT_INACTIVE);
        });
    }

    template <std::size_t N> void SetInput(const std::array<const gpio_dt_spec*, N>& ary)
    {
        std::ranges::for_each(
            ary.begin(), ary.end(), [](const gpio_dt_spec* item) { gpio_pin_configure_dt(item, GPIO_INPUT); });
    }
} // namespace

bool HardwareManager::SetupHardware() noexcept
{
    const std::array outputs = std::to_array(
        {&pwrOff, &led0, &led1, &led2, &led3, &led4, &le, &clk1, &clk2, &serin1, &serin2, &rfCtrl, &rfCtrlN});
    const std::array inputs = std::to_array({&btnGpio});

    if (not IsReady(inputs)) {
        return false;
    }
    if (not IsReady(outputs)) {
        return false;
    }
    SetInactive(outputs);
    SetInput(inputs);

    if (SetupUsb() < 0) {
        return false;
    }

    if (not rgbLed_.Ready()) {
        return false;
    }

    return true;
}

int HardwareManager::SetupUsb()
{
    const device* kUsbSerialPort0 = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
    if (!device_is_ready(kUsbSerialPort0)) {
        return -ENODEV;
    }
    const device* kUsbSerialPort1 = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart1));
    if (!device_is_ready(kUsbSerialPort1)) {
        return -ENODEV;
    }

    int err;

    err = usbd_add_descriptor(&usbdevice, &usbdlang);
    if (err) {
        return err;
    }

    err = usbd_add_descriptor(&usbdevice, &usbdeviceManufacturer);
    if (err) {
        return err;
    }

    err = usbd_add_descriptor(&usbdevice, &usbdeviceProduct);
    if (err) {
        return err;
    }

    err = usbd_add_configuration(&usbdevice, USBD_SPEED_FS, &usbdeviceConfig);
    if (err) {
        return err;
    }

    err = usbd_register_all_classes(&usbdevice, USBD_SPEED_FS, 1, nullptr);
    if (err) {
        return err;
    }

    err = usbd_init(&usbdevice);
    if (err) {
        return err;
    }

    return usbd_enable(&usbdevice);
}

void HardwareManager::ProcessEvent(const msgbus::SystemEvents::EventVariant& event) noexcept
{
    std::visit(
        [this](const auto& concrete_event) {
            if constexpr (requires { Handle(concrete_event); }) {
                Handle(concrete_event);
            }
        },
        event);
}

void HardwareManager::Run()
{
    msgbus::SystemEvents::EventVariant event;
    while (true) {
        if (k_msgq_get(&msgQueue_, &event, K_FOREVER) == 0) {
            ProcessEvent(event);
        }
    }
}
void HardwareManager::RfSwitchSetPath(bool antennaBypass)
{
    gpio_pin_set_dt(&rfCtrl, antennaBypass ? 0 : 1);
    gpio_pin_set_dt(&rfCtrlN, antennaBypass ? 0 : 1);
}

void HardwareManager::SetAttenuator(uint8_t value1, uint8_t value2)
{
    gpio_pin_set_dt(&clk1, 0);
    gpio_pin_set_dt(&serin1, 0);
    gpio_pin_set_dt(&clk2, 0);
    gpio_pin_set_dt(&serin2, 0);

    for (unsigned i = 0; i < 6; i++) {
        const auto shift       = i;
        const auto val1Shifted = (value1 & (0b100000U >> shift)) != 0;
        const auto val2Shifted = (value2 & (0b100000U >> shift)) != 0;
        gpio_pin_set_dt(&serin1, val1Shifted);
        gpio_pin_set_dt(&serin2, val2Shifted);
        k_busy_wait(1);
        gpio_pin_set_dt(&clk1, 1);
        gpio_pin_set_dt(&clk2, 1);
        k_busy_wait(1);
        gpio_pin_set_dt(&clk1, 0);
        gpio_pin_set_dt(&clk2, 0);
        k_busy_wait(1);
        gpio_pin_set_dt(&serin1, 0);
        gpio_pin_set_dt(&serin2, 0);
        k_busy_wait(10);
    }

    k_busy_wait(6);
    gpio_pin_set_dt(&le, 1);
    k_busy_wait(6);
    gpio_pin_set_dt(&le, 0);
}

void HardwareManager::SetPinPowerOff()
{
    gpio_pin_set_dt(&pwrOff, 1);
}