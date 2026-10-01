#include "PeripheralSimulator.h"

#include <simpleble/Exceptions.h>

#include <chrono>
#include <future>

#include "AdapterSimulator.h"
#include "CharacteristicBase.h"
#include "CommonUtils.h"
#include "core/ConnectionCore.h"
#include "DescriptorBase.h"
#include "core/EnvironmentCore.h"
#include "LoggingInternal.h"
#include "ServiceBase.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Simulation;
using namespace SimpleBLE::Simulation::Internal;
using namespace std::chrono_literals;

namespace {

constexpr auto CONNECT_TIMEOUT = 10s;
constexpr auto DISCONNECT_TIMEOUT = 5s;
// The ATT transaction timeout from the Bluetooth Core specification.
constexpr auto ATT_TIMEOUT = 30s;

void check(AttStatus status, const std::string& operation, const BluetoothUUID& uuid) {
    if (status == AttStatus::SUCCESS) return;
    throw Exception::OperationFailed(
        fmt::format("{} on '{}' failed with ATT error 0x{:02X}", operation, uuid, static_cast<uint8_t>(status)));
}

}  // namespace

PeripheralSimulator::PeripheralSimulator(std::weak_ptr<EnvironmentCore> environment,
                                         std::weak_ptr<AdapterSimulator> adapter, std::weak_ptr<DeviceCore> device,
                                         Advertisement advertisement)
    : environment_(std::move(environment)),
      adapter_(std::move(adapter)),
      device_(std::move(device)),
      advertisement_(std::move(advertisement)) {}

void* PeripheralSimulator::underlying() const { return nullptr; }

std::string PeripheralSimulator::identifier() {
    std::lock_guard<std::mutex> lock(mutex_);
    return advertisement_.name;
}

BluetoothAddress PeripheralSimulator::address() {
    std::lock_guard<std::mutex> lock(mutex_);
    return advertisement_.address;
}

BluetoothAddressType PeripheralSimulator::address_type() { return BluetoothAddressType::PUBLIC; }

int16_t PeripheralSimulator::rssi() {
    std::lock_guard<std::mutex> lock(mutex_);
    return advertisement_.rssi;
}

int16_t PeripheralSimulator::tx_power() {
    std::lock_guard<std::mutex> lock(mutex_);
    return advertisement_.tx_power;
}

uint16_t PeripheralSimulator::mtu() {
    auto link = connection();
    if (!link) return 0;
    std::lock_guard<std::mutex> lock(link->mutex);
    return link->open ? link->mtu - 3 : 0;
}

void PeripheralSimulator::connect() {
    if (is_connected()) return;

    auto environment = environment_.lock();
    auto device = device_.lock();
    auto adapter = adapter_.lock();
    if (!environment || !device || !adapter) {
        throw Exception::OperationFailed("The simulated device is no longer available");
    }

    auto attempt = std::make_shared<ConnectAttempt>();
    attempt->adapter = adapter;
    attempt->central = weak_from_this();
    auto ready = environment->request_connection(device, attempt);

    if (ready.wait_for(CONNECT_TIMEOUT) != std::future_status::ready) {
        environment->cancel_connection(attempt);
        throw Exception::OperationFailed("Timed out while connecting to the simulated device");
    }

    std::shared_ptr<ConnectionCore> link;
    try {
        link = ready.get();
    } catch (const std::future_error&) {
        throw Exception::OperationFailed("The simulated connection closed before it was established");
    }

    std::vector<ServiceData> services;
    {
        std::lock_guard<std::mutex> lock(device->mutex);
        services = device->services;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = link;
        services_ = std::move(services);
    }

    if (!link->is_open()) {
        throw Exception::OperationFailed("The simulated connection closed while it was being established");
    }

    adapter->executor()->enqueue([self = shared_from_this()]() { SAFE_CALLBACK_CALL(self->callback_on_connected_); });
}

void PeripheralSimulator::disconnect() {
    auto link = connection();
    if (!link || !link->is_open()) return;

    if (auto environment = environment_.lock()) {
        environment->terminate(link, false);
    }

    if (link->closed.wait_for(DISCONNECT_TIMEOUT) != std::future_status::ready) {
        throw Exception::OperationFailed("Timed out while disconnecting from the simulated device");
    }
}

bool PeripheralSimulator::is_connected() {
    auto link = connection();
    return link && link->is_open();
}

bool PeripheralSimulator::is_connectable() {
    std::lock_guard<std::mutex> lock(mutex_);
    return advertisement_.connectable;
}

bool PeripheralSimulator::is_paired() { return false; }

void PeripheralSimulator::unpair() {}

SharedPtrVector<ServiceBase> PeripheralSimulator::available_services() {
    if (!is_connected()) return {};

    std::lock_guard<std::mutex> lock(mutex_);
    SharedPtrVector<ServiceBase> service_list;
    for (auto& service : services_) {
        SharedPtrVector<CharacteristicBase> characteristic_list;
        for (auto& characteristic : service.characteristics) {
            SharedPtrVector<DescriptorBase> descriptor_list;
            if (characteristic.subscribable()) {
                descriptor_list.push_back(std::make_shared<DescriptorBase>(CCCD_UUID));
            }
            for (auto& [descriptor, value] : characteristic.descriptors) {
                descriptor_list.push_back(std::make_shared<DescriptorBase>(descriptor));
            }
            characteristic_list.push_back(std::make_shared<CharacteristicBase>(
                characteristic.uuid, descriptor_list, characteristic.has(Property::READ),
                characteristic.has(Property::WRITE_REQUEST), characteristic.has(Property::WRITE_COMMAND),
                characteristic.has(Property::NOTIFY), characteristic.has(Property::INDICATE)));
        }
        service_list.push_back(std::make_shared<ServiceBase>(service.uuid, characteristic_list));
    }
    return service_list;
}

SharedPtrVector<ServiceBase> PeripheralSimulator::advertised_services() {
    std::lock_guard<std::mutex> lock(mutex_);
    SharedPtrVector<ServiceBase> service_list;
    for (auto& [service, data] : advertisement_.service_data) {
        service_list.push_back(std::make_shared<ServiceBase>(service, data));
    }
    for (auto& service : advertisement_.services) {
        if (advertisement_.service_data.count(service) == 0) {
            service_list.push_back(std::make_shared<ServiceBase>(service));
        }
    }
    return service_list;
}

std::map<uint16_t, ByteArray> PeripheralSimulator::manufacturer_data() {
    std::lock_guard<std::mutex> lock(mutex_);
    return advertisement_.manufacturer_data;
}

ByteArray PeripheralSimulator::read(BluetoothUUID const& service, BluetoothUUID const& characteristic) {
    if (properties(service, characteristic).count(Property::READ) == 0) {
        throw Exception::OperationNotSupported("read", characteristic);
    }

    Pdu pdu{PduType::READ, normalize(service), normalize(characteristic)};
    auto [status, value] = transact(std::move(pdu));
    check(status, "Read", characteristic);
    return value;
}

void PeripheralSimulator::write_request(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                        ByteArray const& data) {
    if (properties(service, characteristic).count(Property::WRITE_REQUEST) == 0) {
        throw Exception::OperationNotSupported("write_request", characteristic);
    }

    Pdu pdu{PduType::WRITE_REQUEST, normalize(service), normalize(characteristic)};
    pdu.value = data;
    auto [status, value] = transact(std::move(pdu));
    check(status, "Write request", characteristic);
}

void PeripheralSimulator::write_command(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                        ByteArray const& data) {
    if (properties(service, characteristic).count(Property::WRITE_COMMAND) == 0) {
        throw Exception::OperationNotSupported("write_command", characteristic);
    }

    auto link = connection();
    Pdu pdu{PduType::WRITE_COMMAND, normalize(service), normalize(characteristic)};
    pdu.value = data;
    if (!link || !link->send_to_device(std::move(pdu))) throw Exception::NotConnected();
}

void PeripheralSimulator::notify(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                 std::function<void(ByteArray payload)> callback) {
    if (properties(service, characteristic).count(Property::NOTIFY) == 0) {
        throw Exception::OperationNotSupported("notify", characteristic);
    }
    subscribe(service, characteristic, std::move(callback), SubscriptionKind::NOTIFY);
}

void PeripheralSimulator::indicate(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                   std::function<void(ByteArray payload)> callback) {
    if (properties(service, characteristic).count(Property::INDICATE) == 0) {
        throw Exception::OperationNotSupported("indicate", characteristic);
    }
    subscribe(service, characteristic, std::move(callback), SubscriptionKind::INDICATE);
}

void PeripheralSimulator::unsubscribe(BluetoothUUID const& service, BluetoothUUID const& characteristic) {
    properties(service, characteristic);
    Key key{normalize(service), normalize(characteristic)};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        value_callbacks_.erase(key);
    }

    Pdu pdu{PduType::WRITE_DESCRIPTOR, key.first, key.second, CCCD_UUID};
    pdu.value = ByteArray{0x00, 0x00};
    auto [status, value] = transact(std::move(pdu));
    check(status, "Unsubscribe", characteristic);
}

ByteArray PeripheralSimulator::read(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                    BluetoothUUID const& descriptor) {
    properties(service, characteristic);
    Pdu pdu{PduType::READ_DESCRIPTOR, normalize(service), normalize(characteristic), normalize(descriptor)};
    auto [status, value] = transact(std::move(pdu));
    if (status == AttStatus::INVALID_HANDLE) throw Exception::DescriptorNotFound(descriptor);
    check(status, "Descriptor read", descriptor);
    return value;
}

void PeripheralSimulator::write(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                BluetoothUUID const& descriptor, ByteArray const& data) {
    properties(service, characteristic);
    Pdu pdu{PduType::WRITE_DESCRIPTOR, normalize(service), normalize(characteristic), normalize(descriptor)};
    pdu.value = data;
    auto [status, value] = transact(std::move(pdu));
    if (status == AttStatus::INVALID_HANDLE) throw Exception::DescriptorNotFound(descriptor);
    check(status, "Descriptor write", descriptor);
}

void PeripheralSimulator::set_callback_on_connected(std::function<void()> on_connected) {
    if (on_connected) {
        callback_on_connected_.load(std::move(on_connected));
    } else {
        callback_on_connected_.unload();
    }
}

void PeripheralSimulator::set_callback_on_disconnected(std::function<void()> on_disconnected) {
    if (on_disconnected) {
        callback_on_disconnected_.load(std::move(on_disconnected));
    } else {
        callback_on_disconnected_.unload();
    }
}

void PeripheralSimulator::update_advertisement(const Advertisement& advertisement) {
    std::lock_guard<std::mutex> lock(mutex_);
    advertisement_ = advertisement;
}

void PeripheralSimulator::handle_value(const std::shared_ptr<ConnectionCore>& link, const BluetoothUUID& service,
                                       const BluetoothUUID& characteristic, const ByteArray& value) {
    std::function<void(ByteArray payload)> callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_ != link) return;
        auto entry = value_callbacks_.find({service, characteristic});
        if (entry == value_callbacks_.end()) return;
        callback = entry->second;
    }
    SAFE_CALLBACK_CALL(callback, value);
}

void PeripheralSimulator::handle_disconnected(const std::shared_ptr<ConnectionCore>& link) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_ != link) return;
        value_callbacks_.clear();
    }
    SAFE_CALLBACK_CALL(callback_on_disconnected_);
}

std::shared_ptr<ConnectionCore> PeripheralSimulator::connection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_;
}

std::set<Property> PeripheralSimulator::properties(const BluetoothUUID& service,
                                                   const BluetoothUUID& characteristic) const {
    auto service_uuid = normalize(service);
    auto characteristic_uuid = normalize(characteristic);

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& service_data : services_) {
        if (service_data.uuid != service_uuid) continue;
        for (auto& characteristic_data : service_data.characteristics) {
            if (characteristic_data.uuid == characteristic_uuid) return characteristic_data.properties;
        }
        throw Exception::CharacteristicNotFound(characteristic);
    }
    throw Exception::ServiceNotFound(service);
}

std::pair<AttStatus, ByteArray> PeripheralSimulator::transact(Pdu pdu) {
    auto link = connection();
    if (!link) throw Exception::NotConnected();

    // Only the response handler owns the promise: if the connection drops the
    // request, the promise breaks and the wait below ends.
    auto promise = std::make_shared<std::promise<std::pair<AttStatus, ByteArray>>>();
    auto future = promise->get_future();
    pdu.on_response = [promise = std::move(promise), link](AttStatus status, ByteArray value) {
        std::lock_guard<std::mutex> lock(link->mutex);
        promise->set_value({status, std::move(value)});
        link->transaction_cv.notify_all();
    };

    const bool sent = link->send_to_device(std::move(pdu));
    // A moved-from std::function can keep its target (libc++ does for small ones), which would keep
    // the promise alive after the connection drops the request.
    pdu.on_response = nullptr;
    if (!sent) throw Exception::NotConnected();

    std::unique_lock<std::mutex> lock(link->mutex);
    if (!link->transaction_cv.wait_for(lock, ATT_TIMEOUT, [&]() {
            return !link->open || future.wait_for(0s) == std::future_status::ready;
        })) {
        throw Exception::OperationFailed("ATT transaction with the simulated device timed out");
    }
    // A device handler may still own the promise after the connection closes.
    if (future.wait_for(0s) != std::future_status::ready) {
        throw Exception::OperationFailed("The simulated device disconnected before responding");
    }
    lock.unlock();
    try {
        return future.get();
    } catch (const std::future_error&) {
        throw Exception::OperationFailed("The simulated device disconnected before responding");
    }
}

void PeripheralSimulator::subscribe(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                                    std::function<void(ByteArray payload)> callback, SubscriptionKind kind) {
    Key key{normalize(service), normalize(characteristic)};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        value_callbacks_[key] = std::move(callback);
    }

    Pdu pdu{PduType::WRITE_DESCRIPTOR, key.first, key.second, CCCD_UUID};
    pdu.value = ByteArray{static_cast<uint8_t>(kind == SubscriptionKind::NOTIFY ? 0x01 : 0x02), 0x00};
    try {
        auto [status, value] = transact(std::move(pdu));
        check(status, "Subscribe", characteristic);
    } catch (...) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_callbacks_.erase(key);
        throw;
    }
}
