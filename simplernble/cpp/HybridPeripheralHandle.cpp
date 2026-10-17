#include "HybridPeripheralHandle.hpp"

#include <cstdint>
#include <limits>

namespace margelo::nitro::simplernble {

namespace {

// NOTE: The core reports a missing RSSI or TX power as -32768; JavaScript gets
// undefined instead.
std::optional<double> optional_power(int16_t value) {
    if (value == std::numeric_limits<int16_t>::min()) return std::nullopt;
    return value;
}


}  // namespace

HybridPeripheralHandle::HybridPeripheralHandle(SimpleBLE::Peripheral peripheral)
    : HybridObject(TAG), peripheral_(std::move(peripheral)) {}

std::string HybridPeripheralHandle::identifier() {
    return guard([&] { return peripheral_.identifier(); });
}

std::string HybridPeripheralHandle::address() {
    return guard([&] { return peripheral_.address(); });
}

AddressType HybridPeripheralHandle::addressType() {
    return guard([&] {
        switch (peripheral_.address_type()) {
            case SimpleBLE::BluetoothAddressType::PUBLIC:
                return AddressType::PUBLIC;
            case SimpleBLE::BluetoothAddressType::RANDOM:
                return AddressType::RANDOM;
            default:
                return AddressType::UNSPECIFIED;
        }
    });
}

std::optional<double> HybridPeripheralHandle::rssi() {
    return guard([&] { return optional_power(peripheral_.rssi()); });
}

std::optional<double> HybridPeripheralHandle::txPower() {
    return guard([&] { return optional_power(peripheral_.tx_power()); });
}

double HybridPeripheralHandle::mtu() {
    return guard([&] { return static_cast<double>(peripheral_.mtu()); });
}

bool HybridPeripheralHandle::isConnected() {
    return guard([&] { return peripheral_.is_connected(); });
}

bool HybridPeripheralHandle::isConnectable() {
    return guard([&] { return peripheral_.is_connectable(); });
}

std::shared_ptr<Promise<bool>> HybridPeripheralHandle::isPaired() {
    auto peripheral = peripheral_;
    return run<bool>(strand_, [peripheral]() mutable { return peripheral.is_paired(); });
}

std::vector<ManufacturerDataEntry> HybridPeripheralHandle::manufacturerData() {
    return guard([&] {
        std::vector<ManufacturerDataEntry> entries;
        for (const auto& [company_id, data] : peripheral_.manufacturer_data()) {
            entries.emplace_back(static_cast<double>(company_id), to_array_buffer(data));
        }
        return entries;
    });
}

std::vector<NativeService> HybridPeripheralHandle::services() {
    return guard([&] {
        std::vector<NativeService> services;
        for (auto& service : peripheral_.services()) {
            std::vector<NativeCharacteristic> characteristics;
            for (auto& characteristic : service.characteristics()) {
                std::vector<std::string> capabilities;
                if (characteristic.can_read()) capabilities.emplace_back("read");
                if (characteristic.can_write_request()) capabilities.emplace_back("writeRequest");
                if (characteristic.can_write_command()) capabilities.emplace_back("writeCommand");
                if (characteristic.can_notify()) capabilities.emplace_back("notify");
                if (characteristic.can_indicate()) capabilities.emplace_back("indicate");

                std::vector<NativeDescriptor> descriptors;
                for (auto& descriptor : characteristic.descriptors()) descriptors.emplace_back(descriptor.uuid());

                characteristics.emplace_back(characteristic.uuid(), std::move(capabilities), std::move(descriptors));
            }
            services.emplace_back(service.uuid(), to_array_buffer(service.data()), std::move(characteristics));
        }
        return services;
    });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::connect() {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral]() mutable { peripheral.connect(); });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::disconnect() {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral]() mutable { peripheral.disconnect(); });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::unpair() {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral]() mutable { peripheral.unpair(); });
}

std::shared_ptr<Promise<std::shared_ptr<ArrayBuffer>>> HybridPeripheralHandle::read(const std::string& service,
                                                                                    const std::string& characteristic) {
    auto peripheral = peripheral_;
    return run<std::shared_ptr<ArrayBuffer>>(strand_, [peripheral, service, characteristic]() mutable {
        return to_array_buffer(peripheral.read(service, characteristic));
    });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::writeRequest(const std::string& service,
                                                                    const std::string& characteristic,
                                                                    const std::shared_ptr<ArrayBuffer>& data) {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral, service, characteristic, value = to_bytes(data)]() mutable {
        peripheral.write_request(service, characteristic, value);
    });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::writeCommand(const std::string& service,
                                                                    const std::string& characteristic,
                                                                    const std::shared_ptr<ArrayBuffer>& data) {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral, service, characteristic, value = to_bytes(data)]() mutable {
        peripheral.write_command(service, characteristic, value);
    });
}

std::shared_ptr<Promise<std::shared_ptr<ArrayBuffer>>> HybridPeripheralHandle::readDescriptor(
    const std::string& service, const std::string& characteristic, const std::string& descriptor) {
    auto peripheral = peripheral_;
    return run<std::shared_ptr<ArrayBuffer>>(strand_, [peripheral, service, characteristic, descriptor]() mutable {
        return to_array_buffer(peripheral.read(service, characteristic, descriptor));
    });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::writeDescriptor(const std::string& service,
                                                                       const std::string& characteristic,
                                                                       const std::string& descriptor,
                                                                       const std::shared_ptr<ArrayBuffer>& data) {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral, service, characteristic, descriptor, value = to_bytes(data)]() mutable {
        peripheral.write(service, characteristic, descriptor, value);
    });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::notify(const std::string& service,
                                                              const std::string& characteristic, double id) {
    auto peripheral = peripheral_;
    auto on_value = on_value_;
    return run<void>(strand_, [peripheral, service, characteristic, id, on_value]() mutable {
        peripheral.notify(service, characteristic, [on_value, id](SimpleBLE::ByteArray value) {
            if (on_value) on_value(id, to_array_buffer(value));
        });
    });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::indicate(const std::string& service,
                                                                const std::string& characteristic, double id) {
    auto peripheral = peripheral_;
    auto on_value = on_value_;
    return run<void>(strand_, [peripheral, service, characteristic, id, on_value]() mutable {
        peripheral.indicate(service, characteristic, [on_value, id](SimpleBLE::ByteArray value) {
            if (on_value) on_value(id, to_array_buffer(value));
        });
    });
}

std::shared_ptr<Promise<void>> HybridPeripheralHandle::unsubscribe(const std::string& service,
                                                                   const std::string& characteristic) {
    auto peripheral = peripheral_;
    return run<void>(strand_, [peripheral, service, characteristic]() mutable { peripheral.unsubscribe(service, characteristic); });
}

// NOTE: The callbacks stay with the native peripheral, which outlives this
// object. Both run on the JavaScript thread, in the order things happened.
void HybridPeripheralHandle::subscribe(const std::function<void(PeripheralEvent)>& onEvent,
                                       const ValueListener& onValue) {
    on_value_ = onValue;
    auto emit = onEvent;
    guard([&] {
        peripheral_.set_callback_on_connected([emit] { emit(PeripheralEvent::CONNECTED); });
        peripheral_.set_callback_on_disconnected([emit] { emit(PeripheralEvent::DISCONNECTED); });
    });
}

}  // namespace margelo::nitro::simplernble

