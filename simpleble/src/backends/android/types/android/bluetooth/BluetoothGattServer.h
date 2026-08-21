#pragma once

#include <vector>

#include "BluetoothDevice.h"
#include "BluetoothGattCharacteristic.h"
#include "BluetoothGattService.h"
#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleBLE::Android {

class BluetoothGattServer {
  public:
    BluetoothGattServer() = default;
    explicit BluetoothGattServer(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> object);

    explicit operator bool() const { return static_cast<bool>(_obj); }

    bool addService(const BluetoothGattService& service);
    void clearServices();
    void close();
    bool sendResponse(const BluetoothDevice& device, int request_id, int status, int offset,
                      const std::vector<uint8_t>& value);
    bool notifyCharacteristicChanged(const BluetoothDevice& device, const BluetoothGattCharacteristic& characteristic,
                                     bool confirm);

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;

    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _add_service;
    static jmethodID _clear_services;
    static jmethodID _close;
    static jmethodID _send_response;
    static jmethodID _notify_characteristic_changed;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<BluetoothGattServer> registrar;
};

}  // namespace SimpleBLE::Android
