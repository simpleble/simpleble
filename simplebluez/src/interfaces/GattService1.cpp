#include "simplebluez/interfaces/GattService1.h"

using namespace SimpleBluez;

const SimpleDBus::AutoRegisterInterface<GattService1> GattService1::registry{
    "org.bluez.GattService1",
    // clang-format off
    [](std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::static_pointer_cast<SimpleDBus::Interface>(std::make_shared<GattService1>(conn, proxy));
    }
    // clang-format on
};

GattService1::GattService1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy)
    : SimpleDBus::Interface(conn, proxy, "org.bluez.GattService1") {}

void GattService1::property_changed(std::string option_name) {}
