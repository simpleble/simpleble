#include "Cache.h"

#include <stdexcept>
#include <utility>

Cache& Cache::get() {
    // NOTE: The cache is never destroyed on purpose. Tearing down SimpleBLE objects during process exit
    // can hang, as on Windows releasing a local peripheral waits for a WinRT thread that is already gone.
    static Cache& instance = *new Cache();
    return instance;
}

int64_t Cache::addAdapter(SimpleBLE::Adapter adapter) {
    const int64_t adapter_id = AdapterWrapper::getHash(adapter);

    std::scoped_lock lock(mutex);
    // NOTE: Creating a second wrapper would clear the callbacks of the first one once it gets destroyed.
    if (cached_adapters.find(adapter_id) == cached_adapters.end()) {
        cached_adapters.emplace(adapter_id, std::make_shared<AdapterWrapper>(adapter));
    }
    return adapter_id;
}

std::shared_ptr<AdapterWrapper> Cache::getAdapter(int64_t adapter_id) const {
    std::scoped_lock lock(mutex);
    auto it = cached_adapters.find(adapter_id);
    if (it == cached_adapters.end()) throw std::runtime_error("Unknown adapter");
    return it->second;
}

int64_t Cache::addPeripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral) {
    const int64_t peripheral_id = PeripheralWrapper::getHash(peripheral);

    std::scoped_lock lock(mutex);
    if (cached_adapters.find(adapter_id) == cached_adapters.end()) throw std::runtime_error("Unknown adapter");

    auto& peripherals = cached_peripherals[adapter_id];
    if (peripherals.find(peripheral_id) == peripherals.end()) {
        peripherals.emplace(peripheral_id, std::make_shared<PeripheralWrapper>(peripheral));
    }
    return peripheral_id;
}

std::shared_ptr<PeripheralWrapper> Cache::getPeripheral(int64_t adapter_id, int64_t peripheral_id) const {
    std::scoped_lock lock(mutex);
    auto adapter_it = cached_peripherals.find(adapter_id);
    if (adapter_it == cached_peripherals.end()) throw std::runtime_error("Unknown peripheral");

    auto peripheral_it = adapter_it->second.find(peripheral_id);
    if (peripheral_it == adapter_it->second.end()) throw std::runtime_error("Unknown peripheral");
    return peripheral_it->second;
}

int64_t Cache::addBackend(SimpleBLE::Backend backend) {
    const int64_t backend_id = static_cast<int64_t>(std::hash<std::string>{}(backend.identifier()));

    std::scoped_lock lock(mutex);
    cached_backends.try_emplace(backend_id, std::move(backend));
    return backend_id;
}

SimpleBLE::Backend Cache::getBackend(int64_t backend_id) const {
    std::scoped_lock lock(mutex);
    auto it = cached_backends.find(backend_id);
    if (it == cached_backends.end()) throw std::runtime_error("Unknown backend");
    return it->second;
}

int64_t Cache::addLocalPeripheral(SimpleBLE::Local::Peripheral peripheral) {
    std::scoped_lock lock(mutex);
    const int64_t peripheral_id = next_local_id++;
    cached_local_peripherals.emplace(peripheral_id, LocalPeripheralEntry{std::move(peripheral), nullptr});
    return peripheral_id;
}

SimpleBLE::Local::Peripheral Cache::getLocalPeripheral(int64_t peripheral_id) const {
    std::scoped_lock lock(mutex);
    auto it = cached_local_peripherals.find(peripheral_id);
    if (it == cached_local_peripherals.end()) throw std::runtime_error("Unknown local peripheral");
    return it->second.peripheral;
}

void Cache::setLocalPeripheralCallback(int64_t peripheral_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleJavaBLE::LocalPeripheralCallback>(callback);

    std::scoped_lock lock(mutex);
    auto it = cached_local_peripherals.find(peripheral_id);
    if (it == cached_local_peripherals.end()) throw std::runtime_error("Unknown local peripheral");
    // The previous callback gets released together with `wrapper`, after the lock is gone.
    std::swap(it->second.callback, wrapper);
}

std::shared_ptr<Org::SimpleJavaBLE::LocalPeripheralCallback> Cache::getLocalPeripheralCallback(
    int64_t peripheral_id) const {
    std::scoped_lock lock(mutex);
    auto it = cached_local_peripherals.find(peripheral_id);
    return it == cached_local_peripherals.end() ? nullptr : it->second.callback;
}

int64_t Cache::addLocalService(SimpleBLE::Local::Service service) {
    std::scoped_lock lock(mutex);
    const int64_t service_id = next_local_id++;
    cached_local_services.emplace(service_id, std::move(service));
    return service_id;
}

SimpleBLE::Local::Service Cache::getLocalService(int64_t service_id) const {
    std::scoped_lock lock(mutex);
    auto it = cached_local_services.find(service_id);
    if (it == cached_local_services.end()) throw std::runtime_error("Unknown local service");
    return it->second;
}

int64_t Cache::addLocalCharacteristic(SimpleBLE::Local::Characteristic characteristic) {
    std::scoped_lock lock(mutex);
    const int64_t characteristic_id = next_local_id++;
    cached_local_characteristics.emplace(characteristic_id,
                                         LocalCharacteristicEntry{std::move(characteristic), nullptr});
    return characteristic_id;
}

SimpleBLE::Local::Characteristic Cache::getLocalCharacteristic(int64_t characteristic_id) const {
    std::scoped_lock lock(mutex);
    auto it = cached_local_characteristics.find(characteristic_id);
    if (it == cached_local_characteristics.end()) throw std::runtime_error("Unknown local characteristic");
    return it->second.characteristic;
}

void Cache::setLocalCharacteristicCallback(int64_t characteristic_id, jobject callback) {
    auto wrapper = std::make_shared<Org::SimpleJavaBLE::LocalCharacteristicCallback>(callback);

    std::scoped_lock lock(mutex);
    auto it = cached_local_characteristics.find(characteristic_id);
    if (it == cached_local_characteristics.end()) throw std::runtime_error("Unknown local characteristic");
    // The previous callback gets released together with `wrapper`, after the lock is gone.
    std::swap(it->second.callback, wrapper);
}

std::shared_ptr<Org::SimpleJavaBLE::LocalCharacteristicCallback> Cache::getLocalCharacteristicCallback(
    int64_t characteristic_id) const {
    std::scoped_lock lock(mutex);
    auto it = cached_local_characteristics.find(characteristic_id);
    return it == cached_local_characteristics.end() ? nullptr : it->second.callback;
}
