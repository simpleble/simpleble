#pragma once

#include <functional>
#include <vector>

#include <kvn_safe_callback.hpp>
#include <kvn_safe_map.hpp>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"
#include "types/android/bluetooth/BluetoothDevice.h"
#include "types/android/bluetooth/BluetoothGattCharacteristic.h"
#include "types/android/bluetooth/BluetoothGattDescriptor.h"
#include "types/android/bluetooth/BluetoothGattService.h"

namespace SimpleBLE::Android::Bridge {

class GattServerCallback {
  public:
    GattServerCallback();
    ~GattServerCallback();

    jobject get() const { return _obj.get(); }

    void set_callback_onConnectionStateChange(std::function<void(BluetoothDevice, int, int)> callback);
    void set_callback_onServiceAdded(std::function<void(int, BluetoothGattService)> callback);
    void set_callback_onCharacteristicReadRequest(
        std::function<void(BluetoothDevice, int, int, BluetoothGattCharacteristic)> callback);
    void set_callback_onCharacteristicWriteRequest(
        std::function<void(BluetoothDevice, int, BluetoothGattCharacteristic, bool, bool, int, std::vector<uint8_t>)>
            callback);
    void set_callback_onDescriptorReadRequest(
        std::function<void(BluetoothDevice, int, int, BluetoothGattDescriptor)> callback);
    void set_callback_onDescriptorWriteRequest(
        std::function<void(BluetoothDevice, int, BluetoothGattDescriptor, bool, bool, int, std::vector<uint8_t>)>
            callback);
    void set_callback_onExecuteWrite(std::function<void(BluetoothDevice, int, bool)> callback);
    void set_callback_onNotificationSent(std::function<void(BluetoothDevice, int)> callback);

    static GattServerCallback* find(jobject thiz);

    kvn::safe_callback<void(BluetoothDevice, int, int)> on_connection_state_change;
    kvn::safe_callback<void(int, BluetoothGattService)> on_service_added;
    kvn::safe_callback<void(BluetoothDevice, int, int, BluetoothGattCharacteristic)> on_characteristic_read;
    kvn::safe_callback<void(BluetoothDevice, int, BluetoothGattCharacteristic, bool, bool, int, std::vector<uint8_t>)>
        on_characteristic_write;
    kvn::safe_callback<void(BluetoothDevice, int, int, BluetoothGattDescriptor)> on_descriptor_read;
    kvn::safe_callback<void(BluetoothDevice, int, BluetoothGattDescriptor, bool, bool, int, std::vector<uint8_t>)>
        on_descriptor_write;
    kvn::safe_callback<void(BluetoothDevice, int, bool)> on_execute_write;
    kvn::safe_callback<void(BluetoothDevice, int)> on_notification_sent;

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;

    static kvn::safe_map<SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>, GattServerCallback*,
                         SimpleJNI::ObjectComparator<SimpleJNI::GlobalRef, jobject>>
        _instances;
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _constructor;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<GattServerCallback> registrar;
};

}  // namespace SimpleBLE::Android::Bridge
