#include "NativeCache.h"

#include <stdexcept>
#include <tuple>

NativeCache& NativeCache::get() {
    static NativeCache cache;
    return cache;
}

int64_t NativeCache::adapter_id(SimpleBLE::Adapter& adapter) {
    return static_cast<int64_t>(std::hash<std::string>{}(adapter.identifier()));
}

int64_t NativeCache::peripheral_id(SimpleBLE::Peripheral& peripheral) {
    return static_cast<int64_t>(std::hash<std::string>{}(peripheral.address()));
}

int64_t NativeCache::add_adapter(SimpleBLE::Adapter adapter) {
    const int64_t id = adapter_id(adapter);
    std::lock_guard<std::mutex> lock(_mutex);
    _adapters.try_emplace(id, AdapterEntry{std::move(adapter), nullptr});
    return id;
}

int64_t NativeCache::add_peripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral) {
    const int64_t id = peripheral_id(peripheral);
    std::lock_guard<std::mutex> lock(_mutex);
    if (_adapters.find(adapter_id) == _adapters.end()) {
        throw std::runtime_error("Unknown adapter");
    }
    _peripherals[adapter_id].try_emplace(id, PeripheralEntry{std::move(peripheral), nullptr});
    return id;
}

SimpleBLE::Adapter NativeCache::adapter(int64_t adapter_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _adapters.find(adapter_id);
    if (adapter == _adapters.end()) throw std::runtime_error("Unknown adapter");
    return adapter->second.adapter;
}

SimpleBLE::Peripheral NativeCache::peripheral(int64_t adapter_id, int64_t peripheral_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _peripherals.find(adapter_id);
    if (adapter == _peripherals.end()) throw std::runtime_error("Unknown adapter");
    auto peripheral = adapter->second.find(peripheral_id);
    if (peripheral == adapter->second.end()) throw std::runtime_error("Unknown peripheral");
    return peripheral->second.peripheral;
}

int64_t NativeCache::add_local_peripheral(SimpleBLE::Local::Peripheral peripheral) {
    std::lock_guard<std::mutex> lock(_mutex);
    const int64_t id = _next_local_id++;
    _local_peripherals.emplace(id, LocalPeripheralEntry{std::move(peripheral), nullptr});
    return id;
}

int64_t NativeCache::add_local_service(SimpleBLE::Local::Service service) {
    std::lock_guard<std::mutex> lock(_mutex);
    const int64_t id = _next_local_id++;
    _local_services.emplace(id, std::move(service));
    return id;
}

int64_t NativeCache::add_local_characteristic(SimpleBLE::Local::Characteristic characteristic) {
    std::lock_guard<std::mutex> lock(_mutex);
    const int64_t id = _next_local_id++;
    _local_characteristics.emplace(id, LocalCharacteristicEntry{std::move(characteristic), nullptr});
    return id;
}

SimpleBLE::Local::Peripheral NativeCache::local_peripheral(int64_t peripheral_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_peripherals.find(peripheral_id);
    if (item == _local_peripherals.end()) throw std::runtime_error("Unknown local peripheral");
    return item->second.peripheral;
}

SimpleBLE::Local::Service NativeCache::local_service(int64_t service_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_services.find(service_id);
    if (item == _local_services.end()) throw std::runtime_error("Unknown local service");
    return item->second;
}

SimpleBLE::Local::Characteristic NativeCache::local_characteristic(int64_t characteristic_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_characteristics.find(characteristic_id);
    if (item == _local_characteristics.end()) throw std::runtime_error("Unknown local characteristic");
    return item->second.characteristic;
}

void NativeCache::set_local_peripheral_callback(int64_t peripheral_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleBLE::Android::LocalPeripheralCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_peripherals.find(peripheral_id);
    if (item == _local_peripherals.end()) throw std::runtime_error("Unknown local peripheral");
    item->second.callback = std::move(wrapper);
}

std::shared_ptr<Org::SimpleBLE::Android::LocalPeripheralCallback> NativeCache::local_peripheral_callback(
    int64_t peripheral_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_peripherals.find(peripheral_id);
    return item == _local_peripherals.end() ? nullptr : item->second.callback;
}

void NativeCache::set_local_characteristic_callback(int64_t characteristic_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleBLE::Android::LocalCharacteristicCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_characteristics.find(characteristic_id);
    if (item == _local_characteristics.end()) throw std::runtime_error("Unknown local characteristic");
    item->second.callback = std::move(wrapper);
}

std::shared_ptr<Org::SimpleBLE::Android::LocalCharacteristicCallback>
NativeCache::local_characteristic_callback(int64_t characteristic_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto item = _local_characteristics.find(characteristic_id);
    return item == _local_characteristics.end() ? nullptr : item->second.callback;
}

void NativeCache::set_adapter_callback(int64_t adapter_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleBLE::Android::AdapterCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _adapters.find(adapter_id);
    if (adapter == _adapters.end()) throw std::runtime_error("Unknown adapter");
    adapter->second.callback = std::move(wrapper);
}

std::shared_ptr<Org::SimpleBLE::Android::AdapterCallback> NativeCache::adapter_callback(int64_t adapter_id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _adapters.find(adapter_id);
    return adapter == _adapters.end() ? nullptr : adapter->second.callback;
}

void NativeCache::set_peripheral_callback(int64_t adapter_id, int64_t peripheral_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleBLE::Android::PeripheralCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto adapter = _peripherals.find(adapter_id);
    if (adapter == _peripherals.end()) throw std::runtime_error("Unknown adapter");
    auto peripheral = adapter->second.find(peripheral_id);
    if (peripheral == adapter->second.end()) throw std::runtime_error("Unknown peripheral");
    peripheral->second.callback = std::move(wrapper);
}

std::shared_ptr<Org::SimpleBLE::Android::PeripheralCallback> NativeCache::peripheral_callback(
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

std::shared_ptr<Org::SimpleBLE::Android::DataCallback> NativeCache::add_data_callback(
    int64_t adapter_id, int64_t peripheral_id, const std::string& service, const std::string& characteristic,
    jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleBLE::Android::DataCallback>(callback);
    std::lock_guard<std::mutex> lock(_mutex);
    auto [entry, inserted] = _data_callbacks.emplace(
        DataCallbackKey{adapter_id, peripheral_id, service, characteristic}, wrapper);
    return inserted ? wrapper : nullptr;
}

void NativeCache::remove_data_callback(int64_t adapter_id, int64_t peripheral_id, const std::string& service,
                                       const std::string& characteristic,
                                       const std::shared_ptr<Org::SimpleBLE::Android::DataCallback>& expected) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto callback = _data_callbacks.find(DataCallbackKey{adapter_id, peripheral_id, service, characteristic});
    if (callback == _data_callbacks.end()) return;
    if (expected && callback->second != expected) return;
    _data_callbacks.erase(callback);
}
