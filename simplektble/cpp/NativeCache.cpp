#include "NativeCache.h"

#include <simpleble/Exceptions.h>

#include <stdexcept>
#include <tuple>

#include "Internal.h"

namespace Org {
namespace SimpleKtBLE {

NativeCache& NativeCache::get() {
    // NOTE: Never destroyed, so SimpleBLE objects are not torn down while the JVM exits.
    static NativeCache& cache = *new NativeCache();
    return cache;
}

// NOTE: Each SimpleBLE object gets an ID the first time the cache sees it, and keeps it until it is removed, so the
// same adapter, device, local service or local characteristic always comes back with the same ID. IDs are never
// reused, so a removed object's ID cannot reach another one. Requires _mutex.
std::pair<int64_t, bool> NativeCache::assign_id(const void* identity) {
    auto [entry, assigned] = _ids.try_emplace(identity, _next_id);
    if (assigned) _next_id++;
    return {entry->second, assigned};
}

std::string NativeCache::add_backend(SimpleBLE::Backend backend) {
    std::string identifier = backend.identifier();
    std::lock_guard<std::mutex> lock(_mutex);
    _backends.try_emplace(identifier, std::move(backend));
    return identifier;
}

SimpleBLE::Backend NativeCache::backend(const std::string& identifier) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto backend = _backends.find(identifier);
    if (backend == _backends.end()) throw std::runtime_error("Unknown backend");
    return backend->second;
}

int64_t NativeCache::add_adapter(SimpleBLE::Adapter adapter) {
    const void* identity = Internal<SimpleBLE::Adapter>::of(adapter);
    std::lock_guard<std::mutex> lock(_mutex);
    auto [id, assigned] = assign_id(identity);
    if (assigned) _adapters.emplace(id, AdapterEntry{std::move(adapter), nullptr});
    return id;
}

int64_t NativeCache::add_peripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral) {
    const void* identity = Internal<SimpleBLE::Peripheral>::of(peripheral);
    std::lock_guard<std::mutex> lock(_mutex);
    if (_adapters.find(adapter_id) == _adapters.end()) throw SimpleBLE::Exception::NotInitialized();
    auto [id, assigned] = assign_id(identity);
    if (assigned) _peripherals[adapter_id].emplace(id, PeripheralEntry{std::move(peripheral), nullptr});
    return id;
}

std::vector<std::shared_ptr<Org::SimpleKtBLE::DataCallback>> NativeCache::remove_adapter(int64_t adapter_id) {
    // NOTE: Declared before the lock, so the SimpleBLE objects are destroyed after it is released. Destroying an
    // adapter can wait for SimpleBLE threads that call into this cache.
    decltype(_adapters)::node_type adapter;
    decltype(_peripherals)::node_type peripherals;
    std::vector<std::shared_ptr<Org::SimpleKtBLE::DataCallback>> ended;
    std::lock_guard<std::mutex> lock(_mutex);
    adapter = _adapters.extract(adapter_id);
    if (adapter) _ids.erase(Internal<SimpleBLE::Adapter>::of(adapter.mapped().adapter));
    peripherals = _peripherals.extract(adapter_id);
    if (peripherals) {
        for (const auto& entry : peripherals.mapped()) {
            _ids.erase(Internal<SimpleBLE::Peripheral>::of(entry.second.peripheral));
        }
    }
    for (auto entry = _data_callbacks.begin(); entry != _data_callbacks.end();) {
        if (entry->first.adapter_id == adapter_id) {
            ended.push_back(std::move(entry->second));
            entry = _data_callbacks.erase(entry);
        } else {
            ++entry;
        }
    }
    return ended;
}

SimpleBLE::Adapter NativeCache::adapter(int64_t adapter_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _adapters.find(adapter_id);
    if (adapter == _adapters.end()) throw SimpleBLE::Exception::NotInitialized();
    return adapter->second.adapter;
}

SimpleBLE::Peripheral NativeCache::peripheral(int64_t adapter_id, int64_t peripheral_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _peripherals.find(adapter_id);
    if (adapter == _peripherals.end()) throw SimpleBLE::Exception::NotInitialized();
    auto peripheral = adapter->second.find(peripheral_id);
    if (peripheral == adapter->second.end()) throw SimpleBLE::Exception::NotInitialized();
    return peripheral->second.peripheral;
}

// NOTE: Local peripherals are never listed, so each one gets a new ID.
int64_t NativeCache::add_local_peripheral(SimpleBLE::Local::Peripheral peripheral) {
    std::lock_guard<std::mutex> lock(_mutex);
    const int64_t id = _next_id++;
    _local_peripherals.emplace(id, LocalPeripheralEntry{std::move(peripheral), nullptr});
    return id;
}

int64_t NativeCache::add_local_service(SimpleBLE::Local::Service service) {
    const void* identity = Internal<SimpleBLE::Local::Service>::of(service);
    std::lock_guard<std::mutex> lock(_mutex);
    auto [id, assigned] = assign_id(identity);
    if (assigned) _local_services.emplace(id, std::move(service));
    return id;
}

int64_t NativeCache::add_local_characteristic(SimpleBLE::Local::Characteristic characteristic) {
    const void* identity = Internal<SimpleBLE::Local::Characteristic>::of(characteristic);
    std::lock_guard<std::mutex> lock(_mutex);
    auto [id, assigned] = assign_id(identity);
    if (assigned) _local_characteristics.emplace(id, LocalCharacteristicEntry{std::move(characteristic), nullptr});
    return id;
}

// NOTE: Each removed object is declared before the lock, so it is destroyed after the lock is released.
void NativeCache::remove_local_peripheral(int64_t peripheral_id) {
    decltype(_local_peripherals)::node_type peripheral;
    std::lock_guard<std::mutex> lock(_mutex);
    peripheral = _local_peripherals.extract(peripheral_id);
}

void NativeCache::remove_local_service(int64_t service_id) {
    decltype(_local_services)::node_type service;
    std::lock_guard<std::mutex> lock(_mutex);
    service = _local_services.extract(service_id);
    if (service) _ids.erase(Internal<SimpleBLE::Local::Service>::of(service.mapped()));
}

void NativeCache::remove_local_characteristic(int64_t characteristic_id) {
    decltype(_local_characteristics)::node_type characteristic;
    std::lock_guard<std::mutex> lock(_mutex);
    characteristic = _local_characteristics.extract(characteristic_id);
    if (characteristic) {
        _ids.erase(Internal<SimpleBLE::Local::Characteristic>::of(characteristic.mapped().characteristic));
    }
}

SimpleBLE::Local::Peripheral NativeCache::local_peripheral(int64_t peripheral_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_peripherals.find(peripheral_id);
    if (item == _local_peripherals.end()) throw SimpleBLE::Exception::NotInitialized();
    return item->second.peripheral;
}

SimpleBLE::Local::Service NativeCache::local_service(int64_t service_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_services.find(service_id);
    if (item == _local_services.end()) throw SimpleBLE::Exception::NotInitialized();
    return item->second;
}

SimpleBLE::Local::Characteristic NativeCache::local_characteristic(int64_t characteristic_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_characteristics.find(characteristic_id);
    if (item == _local_characteristics.end()) throw SimpleBLE::Exception::NotInitialized();
    return item->second.characteristic;
}

void NativeCache::set_local_peripheral_callback(int64_t peripheral_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleKtBLE::LocalPeripheralCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_peripherals.find(peripheral_id);
    if (item == _local_peripherals.end()) throw SimpleBLE::Exception::NotInitialized();
    item->second.callback = std::move(wrapper);
}

std::shared_ptr<Org::SimpleKtBLE::LocalPeripheralCallback> NativeCache::local_peripheral_callback(
    int64_t peripheral_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_peripherals.find(peripheral_id);
    return item == _local_peripherals.end() ? nullptr : item->second.callback;
}

void NativeCache::set_local_characteristic_callback(int64_t characteristic_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleKtBLE::LocalCharacteristicCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_characteristics.find(characteristic_id);
    if (item == _local_characteristics.end()) throw SimpleBLE::Exception::NotInitialized();
    item->second.callback = std::move(wrapper);
}

std::shared_ptr<Org::SimpleKtBLE::LocalCharacteristicCallback>
NativeCache::local_characteristic_callback(int64_t characteristic_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_characteristics.find(characteristic_id);
    return item == _local_characteristics.end() ? nullptr : item->second.callback;
}

void NativeCache::set_adapter_callback(int64_t adapter_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleKtBLE::AdapterCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _adapters.find(adapter_id);
    if (adapter == _adapters.end()) throw SimpleBLE::Exception::NotInitialized();
    adapter->second.callback = std::move(wrapper);
}

std::shared_ptr<Org::SimpleKtBLE::AdapterCallback> NativeCache::adapter_callback(int64_t adapter_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _adapters.find(adapter_id);
    return adapter == _adapters.end() ? nullptr : adapter->second.callback;
}

void NativeCache::set_peripheral_callback(int64_t adapter_id, int64_t peripheral_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleKtBLE::PeripheralCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _peripherals.find(adapter_id);
    if (adapter == _peripherals.end()) throw SimpleBLE::Exception::NotInitialized();
    auto peripheral = adapter->second.find(peripheral_id);
    if (peripheral == adapter->second.end()) throw SimpleBLE::Exception::NotInitialized();
    peripheral->second.callback = std::move(wrapper);
}

std::shared_ptr<Org::SimpleKtBLE::PeripheralCallback> NativeCache::peripheral_callback(
    int64_t adapter_id, int64_t peripheral_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _peripherals.find(adapter_id);
    if (adapter == _peripherals.end()) return nullptr;
    auto peripheral = adapter->second.find(peripheral_id);
    return peripheral == adapter->second.end() ? nullptr : peripheral->second.callback;
}

bool NativeCache::DataCallbackKey::operator<(const DataCallbackKey& other) const {
    return std::tie(adapter_id, peripheral_id, service, characteristic) <
           std::tie(other.adapter_id, other.peripheral_id, other.service, other.characteristic);
}

std::shared_ptr<Org::SimpleKtBLE::DataCallback> NativeCache::add_data_callback(
    int64_t adapter_id, int64_t peripheral_id, const std::string& service, const std::string& characteristic,
    jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleKtBLE::DataCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto [entry, inserted] = _data_callbacks.emplace(
        DataCallbackKey{adapter_id, peripheral_id, service, characteristic}, wrapper);
    return inserted ? wrapper : nullptr;
}

void NativeCache::remove_data_callback(int64_t adapter_id, int64_t peripheral_id, const std::string& service,
                                       const std::string& characteristic,
                                       const std::shared_ptr<Org::SimpleKtBLE::DataCallback>& expected) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto callback = _data_callbacks.find(DataCallbackKey{adapter_id, peripheral_id, service, characteristic});
    if (callback == _data_callbacks.end()) return;
    if (expected && callback->second != expected) return;
    _data_callbacks.erase(callback);
}

std::vector<std::shared_ptr<Org::SimpleKtBLE::DataCallback>> NativeCache::take_data_callbacks(int64_t adapter_id,
                                                                                             int64_t peripheral_id) {
    std::vector<std::shared_ptr<Org::SimpleKtBLE::DataCallback>> taken;
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto entry = _data_callbacks.begin(); entry != _data_callbacks.end();) {
        if (entry->first.adapter_id == adapter_id && entry->first.peripheral_id == peripheral_id) {
            taken.push_back(std::move(entry->second));
            entry = _data_callbacks.erase(entry);
        } else {
            ++entry;
        }
    }
    return taken;
}

}  // namespace SimpleKtBLE
}  // namespace Org
