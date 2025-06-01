#include "simplebluez/interfaces/Battery1.h"

#include <iostream>

using namespace SimpleBluez;

Battery1::Battery1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path)
    : SimpleDBus::Interface(conn, "org.bluez", path, "org.bluez.Battery1") {}

Battery1::~Battery1() { OnPercentageChanged.unload(); }

void Battery1::property_changed(std::string option_name) {
    if (option_name == "Percentage") {
        OnPercentageChanged();
    }
}
