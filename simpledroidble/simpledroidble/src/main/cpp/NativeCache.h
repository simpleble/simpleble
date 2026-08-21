#pragma once

#include <jni.h>

#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "org/simpleble/android/AdapterCallback.h"
#include "org/simpleble/android/DataCallback.h"
#include "org/simpleble/android/PeripheralCallback.h"
#include "org/simpleble/android/LocalCharacteristicCallback.h"
#include "org/simpleble/android/LocalPeripheralCallback.h"

class NativeCache {
  public:
    static NativeCache& get();

    int64_t add_adapter(SimpleBLE::Adapter adapter);
    int64_t add_peripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral);

    SimpleBLE::Adapter adapter(int64_t adapter_id) const;
    SimpleBLE::Peripheral peripheral(int64_t adapter_id, int64_t peripheral_id) const;

    int64_t add_local_peripheral(SimpleBLE::Local::Peripheral peripheral);
    int64_t add_local_service(SimpleBLE::Local::Service service);
    int64_t add_local_characteristic(SimpleBLE::Local::Characteristic characteristic);
    SimpleBLE::Local::Peripheral local_peripheral(int64_t peripheral_id) const;
    SimpleBLE::Local::Service local_service(int64_t service_id) const;
    SimpleBLE::Local::Characteristic local_characteristic(int64_t characteristic_id) const;

    void set_local_peripheral_callback(int64_t peripheral_id, jobject callback);
    std::shared_ptr<Org::SimpleBLE::Android::LocalPeripheralCallback> local_peripheral_callback(
        int64_t peripheral_id) const;
    void set_local_characteristic_callback(int64_t characteristic_id, jobject callback);
    std::shared_ptr<Org::SimpleBLE::Android::LocalCharacteristicCallback> local_characteristic_callback(
        int64_t characteristic_id) const;

    void set_adapter_callback(int64_t adapter_id, jobject callback);
    std::shared_ptr<Org::SimpleBLE::Android::AdapterCallback> adapter_callback(int64_t adapter_id) const;

    void set_peripheral_callback(int64_t adapter_id, int64_t peripheral_id, jobject callback);
    std::shared_ptr<Org::SimpleBLE::Android::PeripheralCallback> peripheral_callback(int64_t adapter_id,
                                                                                      int64_t peripheral_id) const;

    std::shared_ptr<Org::SimpleBLE::Android::DataCallback> add_data_callback(int64_t adapter_id,
                                                                             int64_t peripheral_id,
                                                                             const std::string& service,
                                                                             const std::string& characteristic,
                                                                             jobject callback);
    void remove_data_callback(int64_t adapter_id, int64_t peripheral_id, const std::string& service,
                              const std::string& characteristic,
                              const std::shared_ptr<Org::SimpleBLE::Android::DataCallback>& expected = nullptr);

  private:
    struct AdapterEntry {
        SimpleBLE::Adapter adapter;
        std::shared_ptr<Org::SimpleBLE::Android::AdapterCallback> callback;
    };

    struct PeripheralEntry {
        SimpleBLE::Peripheral peripheral;
        std::shared_ptr<Org::SimpleBLE::Android::PeripheralCallback> callback;
    };

    struct DataCallbackKey {
        int64_t adapter_id;
        int64_t peripheral_id;
        std::string service;
        std::string characteristic;

        bool operator<(const DataCallbackKey& other) const;
    };

    struct LocalPeripheralEntry {
        SimpleBLE::Local::Peripheral peripheral;
        std::shared_ptr<Org::SimpleBLE::Android::LocalPeripheralCallback> callback;
    };

    struct LocalCharacteristicEntry {
        SimpleBLE::Local::Characteristic characteristic;
        std::shared_ptr<Org::SimpleBLE::Android::LocalCharacteristicCallback> callback;
    };

    NativeCache() = default;

    static int64_t adapter_id(SimpleBLE::Adapter& adapter);
    static int64_t peripheral_id(SimpleBLE::Peripheral& peripheral);

    mutable std::mutex _mutex;
    std::map<int64_t, AdapterEntry> _adapters;
    std::map<int64_t, std::map<int64_t, PeripheralEntry>> _peripherals;
    std::map<DataCallbackKey, std::shared_ptr<Org::SimpleBLE::Android::DataCallback>> _data_callbacks;
    int64_t _next_local_id{1};
    std::map<int64_t, LocalPeripheralEntry> _local_peripherals;
    std::map<int64_t, SimpleBLE::Local::Service> _local_services;
    std::map<int64_t, LocalCharacteristicEntry> _local_characteristics;
};
