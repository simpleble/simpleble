#pragma once

#include <jni.h>

#include <simpleble/Backend.h>
#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "org/simplektble/AdapterCallback.h"
#include "org/simplektble/DataCallback.h"
#include "org/simplektble/LocalCharacteristicCallback.h"
#include "org/simplektble/LocalPeripheralCallback.h"
#include "org/simplektble/PeripheralCallback.h"

namespace Org {
namespace SimpleKtBLE {

class NativeCache {
  public:
    static NativeCache& get();

    std::string add_backend(SimpleBLE::Backend backend);
    SimpleBLE::Backend backend(const std::string& identifier) const;

    int64_t add_adapter(SimpleBLE::Adapter adapter);
    int64_t add_peripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral);

    SimpleBLE::Adapter adapter(int64_t adapter_id) const;
    SimpleBLE::Peripheral peripheral(int64_t adapter_id, int64_t peripheral_id) const;

    /** Forgets an adapter and its peripherals, and returns the data callbacks they had. */
    std::vector<std::shared_ptr<Org::SimpleKtBLE::DataCallback>> remove_adapter(int64_t adapter_id);

    int64_t add_local_peripheral(SimpleBLE::Local::Peripheral peripheral);
    int64_t add_local_service(SimpleBLE::Local::Service service);
    int64_t add_local_characteristic(SimpleBLE::Local::Characteristic characteristic);
    SimpleBLE::Local::Peripheral local_peripheral(int64_t peripheral_id) const;
    SimpleBLE::Local::Service local_service(int64_t service_id) const;
    SimpleBLE::Local::Characteristic local_characteristic(int64_t characteristic_id) const;
    void remove_local_peripheral(int64_t peripheral_id);
    void remove_local_service(int64_t service_id);
    void remove_local_characteristic(int64_t characteristic_id);

    void set_local_peripheral_callback(int64_t peripheral_id, jobject callback);
    std::shared_ptr<Org::SimpleKtBLE::LocalPeripheralCallback> local_peripheral_callback(
        int64_t peripheral_id) const;
    void set_local_characteristic_callback(int64_t characteristic_id, jobject callback);
    std::shared_ptr<Org::SimpleKtBLE::LocalCharacteristicCallback> local_characteristic_callback(
        int64_t characteristic_id) const;

    void set_adapter_callback(int64_t adapter_id, jobject callback);
    std::shared_ptr<Org::SimpleKtBLE::AdapterCallback> adapter_callback(int64_t adapter_id) const;

    void set_peripheral_callback(int64_t adapter_id, int64_t peripheral_id, jobject callback);
    std::shared_ptr<Org::SimpleKtBLE::PeripheralCallback> peripheral_callback(int64_t adapter_id,
                                                                                      int64_t peripheral_id) const;

    std::shared_ptr<Org::SimpleKtBLE::DataCallback> add_data_callback(int64_t adapter_id,
                                                                             int64_t peripheral_id,
                                                                             const std::string& service,
                                                                             const std::string& characteristic,
                                                                             jobject callback);
    void remove_data_callback(int64_t adapter_id, int64_t peripheral_id, const std::string& service,
                              const std::string& characteristic,
                              const std::shared_ptr<Org::SimpleKtBLE::DataCallback>& expected = nullptr);
    /** Removes and returns every data callback of a peripheral. */
    std::vector<std::shared_ptr<Org::SimpleKtBLE::DataCallback>> take_data_callbacks(int64_t adapter_id,
                                                                                    int64_t peripheral_id);

  private:
    struct AdapterEntry {
        SimpleBLE::Adapter adapter;
        std::shared_ptr<Org::SimpleKtBLE::AdapterCallback> callback;
    };

    struct PeripheralEntry {
        SimpleBLE::Peripheral peripheral;
        std::shared_ptr<Org::SimpleKtBLE::PeripheralCallback> callback;
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
        std::shared_ptr<Org::SimpleKtBLE::LocalPeripheralCallback> callback;
    };

    struct LocalCharacteristicEntry {
        SimpleBLE::Local::Characteristic characteristic;
        std::shared_ptr<Org::SimpleKtBLE::LocalCharacteristicCallback> callback;
    };

    NativeCache() = default;

    std::pair<int64_t, bool> assign_id(const void* identity);

    mutable std::mutex _mutex;
    std::map<std::string, SimpleBLE::Backend> _backends;
    std::map<int64_t, AdapterEntry> _adapters;
    std::map<int64_t, std::map<int64_t, PeripheralEntry>> _peripherals;
    std::map<DataCallbackKey, std::shared_ptr<Org::SimpleKtBLE::DataCallback>> _data_callbacks;
    // The ID of each SimpleBLE object in the cache, by the address of SimpleBLE's own object.
    std::map<const void*, int64_t> _ids;
    int64_t _next_id{1};
    std::map<int64_t, LocalPeripheralEntry> _local_peripherals;
    std::map<int64_t, ::SimpleBLE::Local::Service> _local_services;
    std::map<int64_t, LocalCharacteristicEntry> _local_characteristics;
};

}  // namespace SimpleKtBLE
}  // namespace Org
