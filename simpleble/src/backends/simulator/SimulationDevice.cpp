#include <simpleble/simulation/Device.h>

#include <simpleble/Exceptions.h>

#include <algorithm>
#include <mutex>

#include "core/ConnectionCore.h"
#include "core/DeviceCore.h"
#include "core/EnvironmentCore.h"
#include "core/SimulatorUtils.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Simulation;
using namespace SimpleBLE::Simulation::Internal;

namespace {

CharacteristicData& find_characteristic(DeviceCore& device, const BluetoothUUID& service,
                                        const BluetoothUUID& characteristic) {
    auto* service_data = device.service(normalize(service));
    if (!service_data) throw Exception::ServiceNotFound(service);
    auto* characteristic_data = service_data->characteristic(normalize(characteristic));
    if (!characteristic_data) throw Exception::CharacteristicNotFound(characteristic);
    return *characteristic_data;
}

ByteArray& find_descriptor(DeviceCore& device, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                           const BluetoothUUID& descriptor) {
    auto* value = find_characteristic(device, service, characteristic).descriptor(normalize(descriptor));
    if (!value) throw Exception::DescriptorNotFound(descriptor);
    return *value;
}

}  // namespace

Device::Device(std::string name, BluetoothAddress address)
    : internal_(std::make_shared<DeviceCore>(std::move(name), std::move(address))) {}

Device::~Device() = default;

std::shared_ptr<DeviceCore> Device::internal() const { return internal_; }

std::string Device::name() const {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    return internal_->name;
}

BluetoothAddress Device::address() const {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    return internal_->address;
}

void Device::set_advertising_interval(std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->advertising_interval = std::max(interval, std::chrono::milliseconds(20));
}

void Device::set_connectable(bool connectable) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->advertisement.connectable = connectable;
}

void Device::set_tx_power(int16_t tx_power) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->advertisement.tx_power = tx_power;
}

void Device::add_advertised_service(const BluetoothUUID& service) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->advertisement.services.push_back(normalize(service));
}

void Device::set_manufacturer_data(uint16_t company_id, const ByteArray& data) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->advertisement.manufacturer_data[company_id] = data;
}

void Device::set_service_data(const BluetoothUUID& service, const ByteArray& data) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->advertisement.service_data[normalize(service)] = data;
}

void Device::set_preferred_connection_interval(std::chrono::microseconds min, std::chrono::microseconds max) {
    auto range = interval_range(min, max);
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->settings.preferred_interval = range;
}

void Device::set_max_mtu(uint16_t mtu) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->settings.max_mtu = std::clamp<uint16_t>(mtu, 23, 517);
}

void Device::set_max_data_length(uint16_t octets) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->settings.max_data_length = std::clamp<uint16_t>(octets, 27, 251);
}

void Device::set_max_event_length(std::chrono::microseconds length) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->settings.max_event_length = std::max(length, std::chrono::microseconds::zero());
}

void Device::set_tx_buffers(size_t count) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->settings.tx_buffers = std::max<size_t>(count, 1);
}

void Device::set_max_connections(size_t count) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->settings.max_connections = std::max<size_t>(count, 1);
}

void Device::add_service(const BluetoothUUID& service) {
    auto uuid = normalize(service);
    std::lock_guard<std::mutex> lock(internal_->mutex);
    if (internal_->service(uuid)) throw Exception::OperationFailed("Service " + service + " already exists");
    internal_->services.push_back(ServiceData{uuid, {}});
}

void Device::add_characteristic(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                                std::set<Property> properties) {
    auto uuid = normalize(characteristic);
    std::lock_guard<std::mutex> lock(internal_->mutex);
    auto* service_data = internal_->service(normalize(service));
    if (!service_data) throw Exception::ServiceNotFound(service);
    if (service_data->characteristic(uuid)) {
        throw Exception::OperationFailed("Characteristic " + characteristic + " already exists");
    }

    CharacteristicData data;
    data.uuid = uuid;
    data.properties = std::move(properties);
    service_data->characteristics.push_back(std::move(data));
}

void Device::add_descriptor(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                            const BluetoothUUID& descriptor) {
    auto uuid = normalize(descriptor);
    if (uuid == CCCD_UUID) {
        throw Exception::OperationFailed(
            "The Client Characteristic Configuration descriptor is managed by the simulation");
    }

    std::lock_guard<std::mutex> lock(internal_->mutex);
    auto& characteristic_data = find_characteristic(*internal_, service, characteristic);
    if (characteristic_data.descriptor(uuid)) return;
    characteristic_data.descriptors.emplace_back(uuid, ByteArray{});
}

ByteArray Device::value(const BluetoothUUID& service, const BluetoothUUID& characteristic) const {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    return find_characteristic(*internal_, service, characteristic).value;
}

void Device::set_value(const BluetoothUUID& service, const BluetoothUUID& characteristic, const ByteArray& value) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    find_characteristic(*internal_, service, characteristic).value = value;
}

ByteArray Device::value(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                        const BluetoothUUID& descriptor) const {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    return find_descriptor(*internal_, service, characteristic, descriptor);
}

void Device::set_value(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                       const BluetoothUUID& descriptor, const ByteArray& value) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    find_descriptor(*internal_, service, characteristic, descriptor) = value;
}

std::vector<Connection> Device::connections() const {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    std::vector<Connection> result;
    for (auto& weak_connection : internal_->connections) {
        auto connection = weak_connection.lock();
        if (!connection) continue;
        result.emplace_back(connection);
    }
    return result;
}

TxStatus Device::notify(const Connection& connection, const BluetoothUUID& service,
                        const BluetoothUUID& characteristic, const ByteArray& value) {
    return send(connection, service, characteristic, value, SubscriptionKind::NOTIFY);
}

TxStatus Device::indicate(const Connection& connection, const BluetoothUUID& service,
                          const BluetoothUUID& characteristic, const ByteArray& value) {
    return send(connection, service, characteristic, value, SubscriptionKind::INDICATE);
}

TxStatus Device::send(const Connection& connection, const BluetoothUUID& service,
                      const BluetoothUUID& characteristic, const ByteArray& value, SubscriptionKind kind) {
    auto link = connection.internal();
    if (!link || link->device != internal_) return TxStatus::NOT_CONNECTED;

    const bool notification = kind == SubscriptionKind::NOTIFY;
    {
        std::lock_guard<std::mutex> lock(internal_->mutex);
        auto& data = find_characteristic(*internal_, service, characteristic);
        if (!data.has(notification ? Property::NOTIFY : Property::INDICATE)) {
            throw Exception::OperationNotSupported(notification ? "notify" : "indicate", characteristic);
        }
    }

    auto key = std::make_pair(normalize(service), normalize(characteristic));
    std::lock_guard<std::mutex> lock(link->mutex);
    if (!link->open) return TxStatus::NOT_CONNECTED;
    auto& subscriptions = notification ? link->notify_subscriptions : link->indicate_subscriptions;
    if (subscriptions.count(key) == 0) return TxStatus::NOT_SUBSCRIBED;
    if (link->tx_in_use >= link->tx_buffers) return TxStatus::BUSY;
    if (!notification && link->indication_pending) return TxStatus::BUSY;

    Pdu pdu{notification ? PduType::NOTIFICATION : PduType::INDICATION, key.first, key.second};
    // A notification or indication carries at most MTU - 3 bytes.
    const size_t max_length = link->mtu - 3u;
    pdu.value = value.size() > max_length ? value.slice(0, max_length) : value;
    pdu.uses_tx_buffer = true;
    link->tx_in_use++;
    if (!notification) {
        link->indication_pending = true;
    }
    link->enqueue_to_central_locked(std::move(pdu));
    return TxStatus::QUEUED;
}

void Device::disconnect(const Connection& connection) {
    auto link = connection.internal();
    if (!link || link->device != internal_) return;
    auto environment = internal_->environment.lock();
    if (!environment) return;
    environment->terminate(link, true);
}

void Device::request_connection_interval(const Connection& connection, std::chrono::microseconds min,
                                         std::chrono::microseconds max) {
    auto link = connection.internal();
    if (!link || link->device != internal_) return;

    auto range = interval_range(min, max);
    Pdu request{PduType::CONNECTION_PARAMETER_REQUEST};
    request.interval_min = range.first;
    request.interval_max = range.second;
    link->send_to_central(std::move(request));
}

void Device::start_timer(int id, std::chrono::milliseconds period) {
    std::shared_ptr<EnvironmentCore> environment;
    {
        std::lock_guard<std::mutex> lock(internal_->mutex);
        internal_->timers[id] = {internal_->next_timer_generation++, std::max(period, std::chrono::milliseconds(1))};
        environment = internal_->environment.lock();
    }
    if (!environment) return;
    environment->start_timer(internal_, id);
}

void Device::stop_timer(int id) {
    std::lock_guard<std::mutex> lock(internal_->mutex);
    internal_->timers.erase(id);
}

void Device::on_connected(Connection) {}

void Device::on_disconnected(Connection) {}

void Device::on_mtu_changed(Connection, uint16_t) {}

void Device::on_connection_interval_changed(Connection, std::chrono::microseconds) {}

ReadResult Device::on_read(Connection, const BluetoothUUID& service, const BluetoothUUID& characteristic) {
    return value(service, characteristic);
}

AttStatus Device::on_write_request(Connection, const BluetoothUUID&, const BluetoothUUID&, const ByteArray&) {
    return AttStatus::SUCCESS;
}

void Device::on_write_command(Connection, const BluetoothUUID&, const BluetoothUUID&, const ByteArray&) {}

void Device::on_subscribed(Connection, const BluetoothUUID&, const BluetoothUUID&, SubscriptionKind) {}

void Device::on_unsubscribed(Connection, const BluetoothUUID&, const BluetoothUUID&) {}

void Device::on_indication_confirmed(Connection, const BluetoothUUID&, const BluetoothUUID&) {}

void Device::on_tx_complete(Connection) {}

ReadResult Device::on_read_descriptor(Connection, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                                      const BluetoothUUID& descriptor) {
    return value(service, characteristic, descriptor);
}

AttStatus Device::on_write_descriptor(Connection, const BluetoothUUID&, const BluetoothUUID&, const BluetoothUUID&,
                                      const ByteArray&) {
    return AttStatus::SUCCESS;
}

void Device::on_timer(int) {}
