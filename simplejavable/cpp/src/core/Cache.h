#pragma once
#include <simpleble/Backend.h>
#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

#include "AdapterWrapper.h"
#include "PeripheralWrapper.h"
#include "org/simplejavable/LocalCharacteristicCallback.h"
#include "org/simplejavable/LocalPeripheralCallback.h"

/**
 * Keeps the native objects behind the IDs that are handed to Java.
 *
 * Both JNI calls and native callback threads use the cache, so all access goes through a lock. Getters return a
 * copy, which means the lock is never held while SimpleBLE or Java code runs. Unknown IDs throw an exception.
 *
 * NOTE: Entries are never removed, so the IDs held by Java objects stay valid for the lifetime of the process.
 */
class Cache {
  private:
    Cache() = default;

    struct LocalPeripheralEntry {
        SimpleBLE::Local::Peripheral peripheral;
        std::shared_ptr<Org::SimpleJavaBLE::LocalPeripheralCallback> callback;
    };

    struct LocalCharacteristicEntry {
        SimpleBLE::Local::Characteristic characteristic;
        std::shared_ptr<Org::SimpleJavaBLE::LocalCharacteristicCallback> callback;
    };

    mutable std::mutex mutex;
    std::map<int64_t, std::shared_ptr<AdapterWrapper>> cached_adapters;
    std::map<int64_t, std::map<int64_t, std::shared_ptr<PeripheralWrapper>>> cached_peripherals;
    std::map<int64_t, SimpleBLE::Backend> cached_backends;

    int64_t next_local_id = 1;
    std::map<int64_t, LocalPeripheralEntry> cached_local_peripherals;
    std::map<int64_t, SimpleBLE::Local::Service> cached_local_services;
    std::map<int64_t, LocalCharacteristicEntry> cached_local_characteristics;

  public:
    static Cache& get();

    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    // Adapter management. Re-enumerating an adapter returns the ID of the wrapper cached first.
    int64_t addAdapter(SimpleBLE::Adapter adapter);
    std::shared_ptr<AdapterWrapper> getAdapter(int64_t adapter_id) const;

    // Peripheral management. Re-adding a peripheral returns the ID of the wrapper cached first.
    int64_t addPeripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral);
    std::shared_ptr<PeripheralWrapper> getPeripheral(int64_t adapter_id, int64_t peripheral_id) const;

    // Backend management
    int64_t addBackend(SimpleBLE::Backend backend);
    SimpleBLE::Backend getBackend(int64_t backend_id) const;

    // Local peripheral management
    int64_t addLocalPeripheral(SimpleBLE::Local::Peripheral peripheral);
    SimpleBLE::Local::Peripheral getLocalPeripheral(int64_t peripheral_id) const;
    void setLocalPeripheralCallback(int64_t peripheral_id, jobject callback);
    std::shared_ptr<Org::SimpleJavaBLE::LocalPeripheralCallback> getLocalPeripheralCallback(
        int64_t peripheral_id) const;

    int64_t addLocalService(SimpleBLE::Local::Service service);
    SimpleBLE::Local::Service getLocalService(int64_t service_id) const;

    int64_t addLocalCharacteristic(SimpleBLE::Local::Characteristic characteristic);
    SimpleBLE::Local::Characteristic getLocalCharacteristic(int64_t characteristic_id) const;
    void setLocalCharacteristicCallback(int64_t characteristic_id, jobject callback);
    std::shared_ptr<Org::SimpleJavaBLE::LocalCharacteristicCallback> getLocalCharacteristicCallback(
        int64_t characteristic_id) const;
};
