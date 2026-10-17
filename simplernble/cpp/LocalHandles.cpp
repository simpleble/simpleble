#include "LocalHandles.hpp"

#include <simpleble/Advanced.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>
#include <stdexcept>
#include <utility>

namespace margelo::nitro::simplernble {

namespace {

using Capability = SimpleBLE::Local::CharacteristicCapability;

// In the order capabilities() lists them.
const std::pair<const char*, Capability> CAPABILITIES[] = {
    {"read", Capability::READ},
    {"writeRequest", Capability::WRITE_REQUEST},
    {"writeCommand", Capability::WRITE_COMMAND},
    {"notify", Capability::NOTIFY},
    {"indicate", Capability::INDICATE},
};

std::set<Capability> to_capabilities(const std::vector<std::string>& names) {
    std::set<Capability> capabilities;
    for (const auto& name : names) {
        auto match = std::find_if(std::begin(CAPABILITIES), std::end(CAPABILITIES),
                                  [&](const auto& capability) { return name == capability.first; });
        if (match == std::end(CAPABILITIES)) throw std::invalid_argument("Unknown capability: " + name);
        capabilities.insert(match->second);
    }
    return capabilities;
}

}  // namespace

// Local characteristic.

HybridLocalCharacteristicHandle::HybridLocalCharacteristicHandle(SimpleBLE::Local::Characteristic characteristic,
                                                                 HostStrand strand)
    : HybridObject(TAG), characteristic_(std::move(characteristic)), strand_(std::move(strand)) {}

std::string HybridLocalCharacteristicHandle::uuid() {
    return guard([&] { return characteristic_.uuid(); });
}

std::vector<std::string> HybridLocalCharacteristicHandle::capabilities() {
    return guard([&] {
        auto capabilities = characteristic_.capabilities();
        std::vector<std::string> names;
        for (const auto& [name, capability] : CAPABILITIES) {
            if (capabilities.count(capability)) names.emplace_back(name);
        }
        return names;
    });
}

std::shared_ptr<ArrayBuffer> HybridLocalCharacteristicHandle::value() {
    return guard([&] { return to_array_buffer(characteristic_.value()); });
}

std::shared_ptr<Promise<void>> HybridLocalCharacteristicHandle::setValue(const std::shared_ptr<ArrayBuffer>& value) {
    auto characteristic = characteristic_;
    return run<void>(strand_, [characteristic, value = to_bytes(value)]() mutable { characteristic.set_value(value); });
}

// NOTE: The core holds a callback's lock while it runs, and a read waits for
// the JavaScript thread, so the handler is replaced on the host's strand rather
// than the JavaScript thread. A handler that fails answers an empty value; the
// TypeScript layer reports its error. Its bytes arrive as numbers, which Nitro
// copies on the JavaScript thread.
std::shared_ptr<Promise<void>> HybridLocalCharacteristicHandle::setReadHandler(
    const std::optional<std::function<std::shared_ptr<Promise<std::shared_ptr<Promise<std::vector<double>>>>>()>>&
        handler) {
    std::function<SimpleBLE::ByteArray()> on_read;
    if (handler) {
        on_read = [handler = *handler]() -> SimpleBLE::ByteArray {
            try {
                auto numbers = handler()->await().get()->await().get();
                SimpleBLE::ByteArray value(numbers.size());
                std::transform(numbers.begin(), numbers.end(), value.begin(),
                               [](double number) { return static_cast<uint8_t>(number); });
                return value;
            } catch (...) {
                return {};
            }
        };
    }
    auto characteristic = characteristic_;
    return run<void>(strand_, [characteristic, on_read]() mutable { characteristic.set_callback_on_read(on_read); });
}

// NOTE: The callbacks stay with the native characteristic, which outlives this object.
void HybridLocalCharacteristicHandle::subscribe(const std::function<void(LocalCharacteristicEvent)>& onEvent,
                                                const std::function<void(const std::shared_ptr<ArrayBuffer>&)>& onWrite) {
    auto emit = onEvent;
    auto write = onWrite;
    guard([&] {
        characteristic_.set_callback_on_write([write](SimpleBLE::ByteArray value) { write(to_array_buffer(value)); });
        characteristic_.set_callback_on_subscribed([emit] { emit(LocalCharacteristicEvent::SUBSCRIBED); });
        characteristic_.set_callback_on_unsubscribed([emit] { emit(LocalCharacteristicEvent::UNSUBSCRIBED); });
    });
}

// Local service.

HybridLocalServiceHandle::HybridLocalServiceHandle(SimpleBLE::Local::Service service, HostStrand strand)
    : HybridObject(TAG), service_(std::move(service)), strand_(std::move(strand)) {}

std::string HybridLocalServiceHandle::uuid() {
    return guard([&] { return service_.uuid(); });
}

std::shared_ptr<HybridLocalCharacteristicHandleSpec> HybridLocalServiceHandle::addCharacteristic(
    const std::string& uuid, const std::vector<std::string>& capabilities) {
    auto set = to_capabilities(capabilities);
    return guard([&] {
        return std::static_pointer_cast<HybridLocalCharacteristicHandleSpec>(
            intern<HybridLocalCharacteristicHandle>(service_.add_characteristic(uuid, set), strand_));
    });
}

std::vector<std::shared_ptr<HybridLocalCharacteristicHandleSpec>> HybridLocalServiceHandle::characteristics() {
    return guard([&] {
        std::vector<std::shared_ptr<HybridLocalCharacteristicHandleSpec>> handles;
        for (auto& characteristic : service_.characteristics()) {
            handles.push_back(intern<HybridLocalCharacteristicHandle>(characteristic, strand_));
        }
        return handles;
    });
}

// Local peripheral.

HybridLocalPeripheralHandle::HybridLocalPeripheralHandle(SimpleBLE::Local::Peripheral peripheral)
    : HybridObject(TAG), peripheral_(std::move(peripheral)) {}

void HybridLocalPeripheralHandle::addAdvertisedService(const std::vector<std::string>& uuids) {
    guard([&] { peripheral_.add_advertised_service(std::vector<SimpleBLE::BluetoothUUID>(uuids.begin(), uuids.end())); });
}

void HybridLocalPeripheralHandle::setAdvertisedManufacturerData(double companyId, const std::shared_ptr<ArrayBuffer>& data) {
    if (!(companyId >= 0 && companyId <= UINT16_MAX) || std::floor(companyId) != companyId) {
        throw std::invalid_argument("companyId must be an integer from 0 to 65535");
    }
    auto value = to_bytes(data);
    guard([&] { peripheral_.set_advertised_manufacturer_data(static_cast<uint16_t>(companyId), value); });
}

void HybridLocalPeripheralHandle::setConnectable(bool connectable) {
    guard([&] { peripheral_.set_connectable(connectable); });
}

// NOTE: Only CoreBluetooth on iOS lets the advertisement carry its own local name.
void HybridLocalPeripheralHandle::setAdvertisementLocalName(const std::optional<std::string>& name) {
    guard([&] {
#if SIMPLEBLE_BACKEND_IOS
        SimpleBLE::Advanced::iOS::set_advertisement_local_name(peripheral_, name);
#else
        (void)name;
        throw SimpleBLE::Exception::OperationNotSupported();
#endif
    });
}

std::shared_ptr<HybridLocalServiceHandleSpec> HybridLocalPeripheralHandle::addService(const std::string& uuid) {
    return guard([&] {
        return std::static_pointer_cast<HybridLocalServiceHandleSpec>(
            intern<HybridLocalServiceHandle>(peripheral_.add_service(uuid), strand_));
    });
}

std::vector<std::shared_ptr<HybridLocalServiceHandleSpec>> HybridLocalPeripheralHandle::services() {
    return guard([&] {
        std::vector<std::shared_ptr<HybridLocalServiceHandleSpec>> handles;
        for (auto& service : peripheral_.services()) handles.push_back(intern<HybridLocalServiceHandle>(service, strand_));
        return handles;
    });
}

void HybridLocalPeripheralHandle::removeAllServices() {
    guard([&] { peripheral_.remove_all_services(); });
}

std::shared_ptr<Promise<void>> HybridLocalPeripheralHandle::start() {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral]() mutable { peripheral.start(); });
}

std::shared_ptr<Promise<void>> HybridLocalPeripheralHandle::stop() {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral]() mutable { peripheral.stop(); });
}

bool HybridLocalPeripheralHandle::isStarted() {
    return guard([&] { return peripheral_.is_started(); });
}

std::shared_ptr<Promise<bool>> HybridLocalPeripheralHandle::isAdvertising() {
    auto peripheral = peripheral_;
    return run<bool>(strand_, [peripheral]() mutable { return peripheral.is_advertising(); });
}

// NOTE: The callbacks stay with the native peripheral, which outlives this object.
void HybridLocalPeripheralHandle::subscribe(const std::function<void(LocalPeripheralEvent, const std::string&)>& listener) {
    auto emit = listener;
    guard([&] {
        peripheral_.set_callback_on_client_connected(
            [emit](SimpleBLE::BluetoothAddress address) { emit(LocalPeripheralEvent::CLIENTCONNECTED, address); });
        peripheral_.set_callback_on_client_disconnected(
            [emit](SimpleBLE::BluetoothAddress address) { emit(LocalPeripheralEvent::CLIENTDISCONNECTED, address); });
    });
}

}  // namespace margelo::nitro::simplernble
