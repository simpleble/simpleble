#include "simplebluez/interfaces/GattService1.h"

using namespace SimpleBluez;

const SimpleDBus::AutoRegisterInterface<GattService1> GattService1::registry{
    "org.bluez.GattService1",
    // clang-format off
    [](std::shared_ptr<SimpleDBus::Connection> conn, const std::string& bus_name, const std::string& path, const SimpleDBus::Holder& options) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::static_pointer_cast<SimpleDBus::Interface>(std::make_shared<GattService1>(conn, path));
    }
    // clang-format on
};

GattService1::GattService1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path)
    : SimpleDBus::Interface(conn, "org.bluez", path, "org.bluez.GattService1") {}

std::string GattService1::UUID() {
    // As the UUID property doesn't change, we can cache it
    std::scoped_lock lock(_property_update_mutex);
    return _uuid;
}

void GattService1::property_changed(std::string option_name) {
    if (option_name == "UUID") {
        std::scoped_lock lock(_property_update_mutex);
        _uuid = _properties["UUID"].get_string();
    }
}
