#include "simplebluez/interfaces/Device1.h"

using namespace SimpleBluez;

Device1::Device1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path)
    : SimpleDBus::Interface(conn, "org.bluez", path, "org.bluez.Device1") {}

Device1::~Device1() {
    OnDisconnected.unload();
    OnServicesResolved.unload();
}

void Device1::Connect() {
    auto msg = create_method_call("Connect");
    _conn->send_with_reply_and_block(msg);
}

void Device1::Disconnect() {
    auto msg = create_method_call("Disconnect");
    _conn->send_with_reply_and_block(msg);
}

void Device1::Pair() {
    auto msg = create_method_call("Pair");
    _conn->send_with_reply_and_block(msg);
}

void Device1::CancelPairing() {
    auto msg = create_method_call("CancelPairing");
    _conn->send_with_reply_and_block(msg);
}

void Device1::property_changed(std::string option_name) {
    if (option_name == "Connected") {
        if (!Connected.get()) {
            OnDisconnected();
        }
    } else if (option_name == "ServicesResolved") {
        if (ServicesResolved.get()) {
            OnServicesResolved();
        }
    } else if (option_name == "ManufacturerData") {
        ManufacturerData.update_cached_property();
    } else if (option_name == "ServiceData") {
        ServiceData.update_cached_property();
    } else if (option_name == "TxPower") {
        TxPower.update_cached_property();
    }
}
