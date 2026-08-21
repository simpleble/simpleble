#pragma once

#include <utility>

#include "AdvertiseData.h"
#include "AdvertiseSettings.h"
#include "bridge/AdvertiseCallback.h"
#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleBLE::Android {

class BluetoothLeAdvertiser {
  public:
    BluetoothLeAdvertiser() = default;
    explicit BluetoothLeAdvertiser(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> object) : _obj(std::move(object)) {}

    explicit operator bool() const { return static_cast<bool>(_obj); }
    void startAdvertising(const AdvertiseSettings& settings, const AdvertiseData& data,
                          const AdvertiseData& scan_response, Bridge::AdvertiseCallback& callback);
    void stopAdvertising(Bridge::AdvertiseCallback& callback);

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _start_advertising;
    static jmethodID _stop_advertising;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<BluetoothLeAdvertiser> registrar;
};

}  // namespace SimpleBLE::Android
