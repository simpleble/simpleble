#include "simplebluez/interfaces/Battery1.h"

#include <iostream>

using namespace SimpleBluez;

const SimpleDBus::AutoRegisterInterface<Battery1> Battery1::registry{
    "org.bluez.Battery1",
    // clang-format off
    [](std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::static_pointer_cast<SimpleDBus::Interface>(std::make_shared<Battery1>(conn, proxy));
    }
    // clang-format on
};

Battery1::Battery1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy)
    : SimpleDBus::Interface(conn, proxy, "org.bluez.Battery1") {}

Battery1::~Battery1() { OnPercentageChanged.unload(); }

// uint8_t Battery1::Percentage() {
//     std::scoped_lock lock(_property_update_mutex);
//     return _properties["Percentage"].get_byte();
// }

void Battery1::property_changed(std::string option_name) {
    if (option_name == "Percentage") {
        OnPercentageChanged();
    }
}
