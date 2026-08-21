#pragma once

#include "BluetoothGattServer.h"
#include "bridge/GattServerCallback.h"
#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"
#include "types/android/content/Context.h"

namespace SimpleBLE::Android {

class BluetoothManager {
  public:
    explicit BluetoothManager(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> object);

    BluetoothGattServer openGattServer(const Context& context, Bridge::GattServerCallback& callback);

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;

    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _open_gatt_server;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<BluetoothManager> registrar;
};

}  // namespace SimpleBLE::Android
