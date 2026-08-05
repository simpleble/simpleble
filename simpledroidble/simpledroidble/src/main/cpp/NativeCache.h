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

class NativeCache {
  public:
    static NativeCache& get();

    int64_t add_adapter(SimpleBLE::Adapter adapter);
    int64_t add_peripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral);

    SimpleBLE::Adapter adapter(int64_t adapter_id) const;
    SimpleBLE::Peripheral peripheral(int64_t adapter_id, int64_t peripheral_id) const;

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

    NativeCache() = default;

    static int64_t adapter_id(SimpleBLE::Adapter& adapter);
    static int64_t peripheral_id(SimpleBLE::Peripheral& peripheral);

    mutable std::mutex _mutex;
    std::map<int64_t, AdapterEntry> _adapters;
    std::map<int64_t, std::map<int64_t, PeripheralEntry>> _peripherals;
    std::map<DataCallbackKey, std::shared_ptr<Org::SimpleBLE::Android::DataCallback>> _data_callbacks;
};
