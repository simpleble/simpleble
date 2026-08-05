#pragma once

#include <jni.h>

#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "JniTypes.h"

class NativeCache {
  public:
    static NativeCache& get();

    int64_t add_adapter(SimpleBLE::Adapter adapter);
    int64_t add_peripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral);

    SimpleBLE::Adapter adapter(int64_t adapter_id) const;
    SimpleBLE::Peripheral peripheral(int64_t adapter_id, int64_t peripheral_id) const;

    void set_adapter_callback(int64_t adapter_id, jobject callback);
    std::shared_ptr<SimpleDroidJNI::AdapterCallback> adapter_callback(int64_t adapter_id) const;

    void set_peripheral_callback(int64_t adapter_id, int64_t peripheral_id, jobject callback);
    std::shared_ptr<SimpleDroidJNI::PeripheralCallback> peripheral_callback(int64_t adapter_id,
                                                                            int64_t peripheral_id) const;

    std::shared_ptr<SimpleDroidJNI::DataCallback> add_data_callback(int64_t adapter_id, int64_t peripheral_id,
                                                                    const std::string& service,
                                                                    const std::string& characteristic,
                                                                    jobject callback);
    void remove_data_callback(int64_t adapter_id, int64_t peripheral_id, const std::string& service,
                              const std::string& characteristic,
                              const std::shared_ptr<SimpleDroidJNI::DataCallback>& expected = nullptr);

  private:
    struct AdapterEntry {
        SimpleBLE::Adapter adapter;
        std::shared_ptr<SimpleDroidJNI::AdapterCallback> callback;
    };

    struct PeripheralEntry {
        SimpleBLE::Peripheral peripheral;
        std::shared_ptr<SimpleDroidJNI::PeripheralCallback> callback;
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
    std::map<DataCallbackKey, std::shared_ptr<SimpleDroidJNI::DataCallback>> _data_callbacks;
};
