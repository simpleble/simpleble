#include <simplebluez/Exceptions.h>
#include <simplebluez/standard/Device.h>
#include <simplebluez/standard/Service.h>
#include <simpledbus/interfaces/Properties.h>
#include "simplebluez/interfaces/Battery1.h"

#include <utility>

using namespace SimpleBluez;

Device::Device(std::shared_ptr<SimpleDBus::Connection> conn, const std::string& bus_name, const std::string& path)
    : Proxy(conn, bus_name, path) {}

Device::~Device() {
    _callback_on_connected.unload();
    _callback_on_disconnected.unload();
    device1()->Connected.on_changed.unload();
}

void Device::on_registration() {
    auto device1 = std::make_shared<Device1>(_conn, shared_from_this());
    device1->Connected.on_changed.load([this](bool connected) {
        if (connected) {
            _callback_on_connected();
        } else {
            _callback_on_disconnected();
        }
    });
    _interfaces.emplace(std::make_pair("org.bluez.Device1", device1));

    auto properties = std::make_shared<SimpleDBus::Interfaces::Properties>(_conn, shared_from_this());
    _interfaces.emplace(std::make_pair("org.freedesktop.DBus.Properties", properties));
}

std::shared_ptr<SimpleDBus::Proxy> Device::path_create(const std::string& path) {
    const std::string next_child = SimpleDBus::PathUtils::next_child_strip(_path, path);

    if (next_child.find("service") == 0) {
        return Proxy::create<Service>(_conn, _bus_name, path);
    } else {
        return Proxy::create<Proxy>(_conn, _bus_name, path);
    }
}

std::shared_ptr<Device1> Device::device1() {
    return std::dynamic_pointer_cast<Device1>(interface_get("org.bluez.Device1"));
}

std::shared_ptr<Battery1> Device::battery1() {
    return std::dynamic_pointer_cast<Battery1>(interface_get("org.bluez.Battery1"));
}

std::vector<std::shared_ptr<Service>> Device::services() { return children_casted_with_prefix<Service>("service"); }

std::shared_ptr<Service> Device::get_service(const std::string& uuid) {
    auto services_all = services();

    for (auto& service : services_all) {
        if (service->uuid() == uuid) {
            return service;
        }
    }

    throw Exception::ServiceNotFoundException(uuid);
}

std::shared_ptr<Characteristic> Device::get_characteristic(const std::string& service_uuid,
                                                           const std::string& characteristic_uuid) {
    auto service = get_service(service_uuid);
    return service->get_characteristic(characteristic_uuid);
}

void Device::pair() { device1()->Pair(); }

void Device::cancel_pairing() { device1()->CancelPairing(); }

void Device::connect() { device1()->Connect(); }

void Device::disconnect() {
    if (!valid()) return;
    device1()->Disconnect();
}

std::string Device::address() { return device1()->Address; }

std::string Device::address_type() { return device1()->AddressType; }

std::string Device::name() { return device1()->Name; }

std::string Device::alias() { return device1()->Alias; }

int16_t Device::rssi() { return device1()->RSSI; }

int16_t Device::tx_power() { return device1()->TxPower; }

std::vector<std::string> Device::uuids() { return device1()->UUIDs.refresh(); }

std::map<uint16_t, ByteArray> Device::manufacturer_data() { return device1()->ManufacturerData.refresh(); }

std::map<std::string, ByteArray> Device::service_data() { return device1()->ServiceData.refresh(); }

bool Device::paired() { return valid() && device1()->Paired.refresh(); }

bool Device::bonded() { return valid() && device1()->Bonded.refresh(); }

bool Device::connected() { return valid() && device1()->Connected.refresh(); }

bool Device::services_resolved() {
    if (!valid()) return false;

    // ServicesResolved is only useful to consumers once SimpleBluez has observed
    // the local property update. A synchronous refresh can see BlueZ's property
    // before the matching GATT objects have been dispatched into the local tree.
    auto& services_resolved = device1()->ServicesResolved;
    return services_resolved.valid() && services_resolved.get();
}

void Device::set_on_connected(std::function<void()> callback) { _callback_on_connected.load(std::move(callback)); }

void Device::clear_on_connected() { _callback_on_connected.unload(); }

void Device::set_on_disconnected(std::function<void()> callback) {
    _callback_on_disconnected.load(std::move(callback));
}

void Device::clear_on_disconnected() { _callback_on_disconnected.unload(); }

void Device::set_on_services_resolved(std::function<void()> callback) {
    device1()->ServicesResolved.on_changed.load([callback](bool services_resolved) {
        if (services_resolved) {
            callback();
        }
    });
}

void Device::clear_on_services_resolved() { device1()->ServicesResolved.on_changed.unload(); }

bool Device::has_battery_interface() { return interface_exists("org.bluez.Battery1"); }

uint8_t Device::battery_percentage() { return battery1()->Percentage(); }

void Device::set_on_battery_percentage_changed(std::function<void(uint8_t new_value)> callback) {
    battery1()->Percentage.on_changed.load([this, callback](uint8_t new_value) { callback(new_value); });
    // As the `property_changed` callback only occurs when the property is changed, we need to manually
    // call the callback once to make sure the callback is called with the current value.
    battery1()->Percentage.notify_changed();
}

void Device::clear_on_battery_percentage_changed() { battery1()->Percentage.on_changed.unload(); }
