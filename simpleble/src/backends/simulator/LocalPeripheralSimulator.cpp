#include "LocalPeripheralSimulator.h"

#include <simpleble/simulation/Device.h>

#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <utility>

#include "AdapterSimulator.h"
#include "CommonUtils.h"
#include "LocalCharacteristicBase.h"
#include "LocalServiceBase.h"
#include "core/ConnectionCore.h"
#include "core/DeviceCore.h"
#include "core/EnvironmentCore.h"
#include "core/SimulatorUtils.h"

namespace SimpleBLE::Local {
namespace {

using namespace Simulation;
using namespace Simulation::Internal;

BluetoothAddress client_address(const Connection& connection) {
    auto link = connection.internal();
    auto adapter = link ? link->adapter.lock() : nullptr;
    return adapter ? adapter->address() : BluetoothAddress{};
}

class LocalDevice;
class ServiceSimulator;

class CharacteristicSimulator final : public CharacteristicBase {
  public:
    CharacteristicSimulator(std::weak_ptr<LocalDevice> device, BluetoothUUID service, BluetoothUUID uuid,
                            std::set<CharacteristicCapability> capabilities)
        : device_(std::move(device)),
          service_(std::move(service)),
          uuid_(std::move(uuid)),
          capabilities_(std::move(capabilities)) {}

    BluetoothUUID uuid() override { return uuid_; }

    std::set<CharacteristicCapability> capabilities() override { return capabilities_; }

    ByteArray value() override;
    void set_value(ByteArray value) override;

    void set_callback_on_read(std::function<ByteArray()> callback) override {
        if (callback) {
            on_read_.load(std::move(callback));
        } else {
            on_read_.unload();
        }
    }

    void set_callback_on_write(std::function<void(ByteArray)> callback) override {
        if (callback) {
            on_write_.load(std::move(callback));
        } else {
            on_write_.unload();
        }
    }

    void set_callback_on_subscribed(std::function<void()> callback) override {
        if (callback) {
            on_subscribed_.load(std::move(callback));
        } else {
            on_subscribed_.unload();
        }
    }

    void set_callback_on_unsubscribed(std::function<void()> callback) override {
        if (callback) {
            on_unsubscribed_.load(std::move(callback));
        } else {
            on_unsubscribed_.unload();
        }
    }

    ByteArray read() { return on_read_ ? on_read_() : value(); }
    void write(const ByteArray& value);
    void subscribe(const std::shared_ptr<ConnectionCore>& connection);
    void unsubscribe(const std::shared_ptr<ConnectionCore>& connection);

  private:
    std::weak_ptr<LocalDevice> device_;
    const BluetoothUUID service_;
    const BluetoothUUID uuid_;
    const std::set<CharacteristicCapability> capabilities_;

    std::mutex mutex_;
    std::set<std::shared_ptr<ConnectionCore>> subscribers_;

    kvn::safe_callback<ByteArray()> on_read_;
    kvn::safe_callback<void(ByteArray)> on_write_;
    kvn::safe_callback<void()> on_subscribed_;
    kvn::safe_callback<void()> on_unsubscribed_;
};

class LocalDevice final : public Device, public std::enable_shared_from_this<LocalDevice> {
  public:
    LocalDevice(std::weak_ptr<EnvironmentCore> environment, const std::string& name, const BluetoothAddress& address)
        : Device(name, address), environment(std::move(environment)) {
        set_max_connections(8);
        set_max_data_length(251);
    }

    void require_stopped() {
        require_alive();
        if (started) throw Exception::OperationFailed("Local GATT configuration cannot change while started");
    }

    void require_alive() {
        if (released) throw Exception::InvalidReference();
    }

    std::shared_ptr<CharacteristicSimulator> find(const BluetoothUUID& service, const BluetoothUUID& characteristic);

    // Caller holds mutex.
    std::shared_ptr<CharacteristicSimulator> find_locked(const BluetoothUUID& service,
                                                         const BluetoothUUID& characteristic);

    void start();
    void stop();

    void on_connected(Connection connection) override;
    void on_disconnected(Connection connection) override;

    ReadResult on_read(Connection, const BluetoothUUID& service, const BluetoothUUID& characteristic) override {
        return find(service, characteristic)->read();
    }

    AttStatus on_write_request(Connection, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                               const ByteArray& value) override {
        find(service, characteristic)->write(value);
        return AttStatus::SUCCESS;
    }

    void on_write_command(Connection connection, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                          const ByteArray& value) override {
        on_write_request(connection, service, characteristic, value);
    }

    void on_subscribed(Connection connection, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                       SubscriptionKind) override {
        find(service, characteristic)->subscribe(connection.internal());
    }

    void on_unsubscribed(Connection connection, const BluetoothUUID& service,
                         const BluetoothUUID& characteristic) override {
        find(service, characteristic)->unsubscribe(connection.internal());
    }

    std::mutex mutex;
    bool started = false;

    // Queued events can outlive the local host.
    std::atomic_bool released{false};

    std::vector<BluetoothUUID> advertised;
    std::map<BluetoothUUID, std::shared_ptr<ServiceSimulator>> services;

    // Services may be replaced before disconnect callbacks run.
    std::map<const ConnectionCore*, std::vector<std::shared_ptr<CharacteristicSimulator>>> client_characteristics;

    std::weak_ptr<EnvironmentCore> environment;
    kvn::safe_callback<void(BluetoothAddress)> on_connected_callback;
    kvn::safe_callback<void(BluetoothAddress)> on_disconnected_callback;
};

class ServiceSimulator final : public ServiceBase {
  public:
    ServiceSimulator(std::weak_ptr<LocalDevice> device, BluetoothUUID uuid)
        : device_(std::move(device)), uuid_(std::move(uuid)) {}

    BluetoothUUID uuid() override { return uuid_; }

    std::shared_ptr<CharacteristicBase> add_characteristic(BluetoothUUID uuid,
                                                           std::set<CharacteristicCapability> capabilities) override {
        auto device = device_.lock();
        if (!device) throw Exception::InvalidReference();

        std::lock_guard<std::mutex> lock(device->mutex);
        device->require_stopped();

        auto service = device->services.find(uuid_);
        if (service == device->services.end() || service->second.get() != this) throw Exception::InvalidReference();

        if (capabilities.empty()) {
            throw Exception::OperationFailed("A local characteristic requires at least one capability");
        }
        uuid = normalize(uuid);

        std::set<Property> properties;
        for (auto capability : capabilities) {
            switch (capability) {
                case CharacteristicCapability::READ:
                    properties.insert(Property::READ);
                    break;
                case CharacteristicCapability::WRITE_REQUEST:
                    properties.insert(Property::WRITE_REQUEST);
                    break;
                case CharacteristicCapability::WRITE_COMMAND:
                    properties.insert(Property::WRITE_COMMAND);
                    break;
                case CharacteristicCapability::NOTIFY:
                    properties.insert(Property::NOTIFY);
                    break;
                case CharacteristicCapability::INDICATE:
                    properties.insert(Property::INDICATE);
                    break;
                default:
                    throw Exception::OperationFailed("Invalid characteristic capability");
            }
        }

        device->add_characteristic(uuid_, uuid, properties);
        auto characteristic = std::make_shared<CharacteristicSimulator>(device, uuid_, uuid, std::move(capabilities));
        characteristics_[uuid] = characteristic;
        return characteristic;
    }

    std::vector<std::shared_ptr<CharacteristicBase>> characteristics() override {
        auto device = device_.lock();
        if (!device) throw Exception::InvalidReference();

        std::lock_guard<std::mutex> lock(device->mutex);
        device->require_alive();

        auto service = device->services.find(uuid_);
        if (service == device->services.end() || service->second.get() != this) throw Exception::InvalidReference();

        std::vector<std::shared_ptr<CharacteristicBase>> result;
        for (auto& [uuid, characteristic] : characteristics_) {
            result.push_back(characteristic);
        }
        return result;
    }

    std::map<BluetoothUUID, std::shared_ptr<CharacteristicSimulator>> characteristics_;

  private:
    std::weak_ptr<LocalDevice> device_;
    const BluetoothUUID uuid_;
};

std::shared_ptr<CharacteristicSimulator> LocalDevice::find(const BluetoothUUID& service,
                                                           const BluetoothUUID& characteristic) {
    std::lock_guard<std::mutex> lock(mutex);
    return find_locked(service, characteristic);
}

std::shared_ptr<CharacteristicSimulator> LocalDevice::find_locked(const BluetoothUUID& service,
                                                                  const BluetoothUUID& characteristic) {
    require_alive();

    auto service_it = services.find(service);
    if (service_it == services.end()) throw Exception::ServiceNotFound(service);

    auto characteristic_it = service_it->second->characteristics_.find(characteristic);
    if (characteristic_it == service_it->second->characteristics_.end()) {
        throw Exception::CharacteristicNotFound(characteristic);
    }
    return characteristic_it->second;
}

void LocalDevice::start() {
    std::lock_guard<std::mutex> lock(mutex);
    if (started) return;

    auto owner = environment.lock();
    if (!owner) throw Exception::InvalidReference();
    if (!owner->is_active()) throw Exception::OperationFailed("The simulation environment is inactive");

    {
        std::lock_guard<std::mutex> core_lock(internal()->mutex);
        internal()->advertisement.services = advertised;
        if (advertised.empty()) {
            for (auto& [uuid, service] : services) {
                internal()->advertisement.services.push_back(uuid);
            }
        }
    }

    owner->add_device(shared_from_this());
    started = true;
}

void LocalDevice::stop() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!started) return;

    if (auto owner = environment.lock()) owner->remove_device(shared_from_this());
    started = false;
}

void LocalDevice::on_connected(Connection connection) {
    if (released) return;

    {
        std::lock_guard<std::mutex> lock(mutex);
        auto& characteristics = client_characteristics[connection.internal().get()];
        for (auto& [uuid, service] : services) {
            for (auto& [uuid, characteristic] : service->characteristics_) {
                characteristics.push_back(characteristic);
            }
        }
    }

    SAFE_CALLBACK_CALL(on_connected_callback, client_address(connection));
}

void LocalDevice::on_disconnected(Connection connection) {
    std::vector<std::shared_ptr<CharacteristicSimulator>> characteristics;
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto client = client_characteristics.find(connection.internal().get());
        if (client != client_characteristics.end()) {
            characteristics = std::move(client->second);
            client_characteristics.erase(client);
        }
    }

    for (auto& characteristic : characteristics) characteristic->unsubscribe(connection.internal());
    if (!released) SAFE_CALLBACK_CALL(on_disconnected_callback, client_address(connection));
}

ByteArray CharacteristicSimulator::value() {
    auto device = device_.lock();
    if (!device) throw Exception::InvalidReference();

    std::lock_guard<std::mutex> lock(device->mutex);
    if (device->find_locked(service_, uuid_).get() != this) throw Exception::InvalidReference();

    return device->value(service_, uuid_);
}

void CharacteristicSimulator::set_value(ByteArray value) {
    auto device = device_.lock();
    if (!device) throw Exception::InvalidReference();

    std::lock_guard<std::mutex> lock(device->mutex);
    if (device->find_locked(service_, uuid_).get() != this) throw Exception::InvalidReference();

    device->set_value(service_, uuid_, value);
    for (auto& connection : device->connections()) {
        if (capabilities_.count(CharacteristicCapability::NOTIFY)) {
            device->notify(connection, service_, uuid_, value);
        }
        if (capabilities_.count(CharacteristicCapability::INDICATE)) {
            device->indicate(connection, service_, uuid_, value);
        }
    }
}

void CharacteristicSimulator::write(const ByteArray& value) {
    auto device = device_.lock();
    if (!device) throw Exception::InvalidReference();

    {
        std::lock_guard<std::mutex> lock(device->mutex);
        if (device->find_locked(service_, uuid_).get() != this) throw Exception::InvalidReference();
        device->set_value(service_, uuid_, value);
    }

    SAFE_CALLBACK_CALL(on_write_, value);
}

void CharacteristicSimulator::subscribe(const std::shared_ptr<ConnectionCore>& connection) {
    bool first;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        first = subscribers_.insert(connection).second && subscribers_.size() == 1;
    }

    if (first) SAFE_CALLBACK_CALL(on_subscribed_);
}

void CharacteristicSimulator::unsubscribe(const std::shared_ptr<ConnectionCore>& connection) {
    bool last;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        last = subscribers_.erase(connection) > 0 && subscribers_.empty();
    }

    auto device = device_.lock();
    if (last && device && !device->released) SAFE_CALLBACK_CALL(on_unsubscribed_);
}

class PeripheralSimulator final : public PeripheralBase {
  public:
    explicit PeripheralSimulator(std::shared_ptr<LocalDevice> device) : device_(std::move(device)) {}

    ~PeripheralSimulator() override {
        device_->released = true;
        device_->on_connected_callback.unload();
        device_->on_disconnected_callback.unload();
        device_->stop();
    }

    void* underlying() const override { return nullptr; }

    void add_advertised_service(BluetoothUUID uuid) override {
        add_advertised_service(std::vector<BluetoothUUID>{uuid});
    }

    void add_advertised_service(std::vector<BluetoothUUID> uuids) override {
        std::lock_guard<std::mutex> lock(device_->mutex);
        device_->require_stopped();

        for (auto& uuid : uuids) {
            device_->advertised.push_back(normalize(uuid));
        }
    }

    std::shared_ptr<ServiceBase> add_service(BluetoothUUID uuid) override {
        std::lock_guard<std::mutex> lock(device_->mutex);
        device_->require_stopped();

        uuid = normalize(uuid);
        device_->add_service(uuid);
        auto service = std::make_shared<ServiceSimulator>(device_, uuid);
        device_->services[uuid] = service;
        return service;
    }

    std::vector<std::shared_ptr<ServiceBase>> services() override {
        std::lock_guard<std::mutex> lock(device_->mutex);
        std::vector<std::shared_ptr<ServiceBase>> result;
        for (auto& [uuid, service] : device_->services) {
            result.push_back(service);
        }
        return result;
    }

    void remove_all_services() override {
        std::lock_guard<std::mutex> lock(device_->mutex);
        device_->require_stopped();

        std::lock_guard<std::mutex> core_lock(device_->internal()->mutex);
        device_->internal()->services.clear();
        device_->services.clear();
    }

    void start() override { device_->start(); }

    void stop() override { device_->stop(); }

    bool is_started() override {
        std::lock_guard<std::mutex> lock(device_->mutex);
        auto environment = device_->environment.lock();
        return device_->started && environment && environment->is_active();
    }

    bool is_advertising() override { return is_started() && device_->connections().size() < 8; }

    void set_callback_on_client_connected(std::function<void(BluetoothAddress)> callback) override {
        if (callback) {
            device_->on_connected_callback.load(std::move(callback));
        } else {
            device_->on_connected_callback.unload();
        }
    }

    void set_callback_on_client_disconnected(std::function<void(BluetoothAddress)> callback) override {
        if (callback) {
            device_->on_disconnected_callback.load(std::move(callback));
        } else {
            device_->on_disconnected_callback.unload();
        }
    }

  private:
    std::shared_ptr<LocalDevice> device_;
};

}  // namespace

std::shared_ptr<PeripheralBase> make_simulated_peripheral(const std::shared_ptr<EnvironmentCore>& environment,
                                                          const std::shared_ptr<AdapterSimulator>& adapter) {
    static std::atomic<uint32_t> next_address{1};
    const auto id = next_address.fetch_add(1);
    auto address = fmt::format("02:00:{:02X}:{:02X}:{:02X}:{:02X}", (id >> 24) & 0xff, (id >> 16) & 0xff,
                               (id >> 8) & 0xff, id & 0xff);

    return std::make_shared<PeripheralSimulator>(
        std::make_shared<LocalDevice>(environment, adapter->identifier() + " Peripheral", address));
}

}  // namespace SimpleBLE::Local
