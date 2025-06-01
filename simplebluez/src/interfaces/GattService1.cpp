#include "simplebluez/interfaces/GattService1.h"

using namespace SimpleBluez;

GattService1::GattService1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path)
    : SimpleDBus::Interface(conn, "org.bluez", path, "org.bluez.GattService1") {}

void GattService1::property_changed(std::string option_name) {
    if (option_name == "UUID") {
        UUID.update_cached_property();
    }
}
