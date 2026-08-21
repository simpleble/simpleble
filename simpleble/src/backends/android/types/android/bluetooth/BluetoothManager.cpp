#include "BluetoothManager.h"

#include <stdexcept>
#include <utility>

namespace SimpleBLE::Android {

SimpleJNI::GlobalRef<jclass> BluetoothManager::_cls;
jmethodID BluetoothManager::_open_gatt_server = nullptr;

const SimpleJNI::JNIDescriptor BluetoothManager::descriptor{
    "android/bluetooth/BluetoothManager",
    &_cls,
    {{"openGattServer",
      "(Landroid/content/Context;Landroid/bluetooth/BluetoothGattServerCallback;)Landroid/bluetooth/"
      "BluetoothGattServer;",
      &_open_gatt_server}}};
const SimpleJNI::AutoRegister<BluetoothManager> BluetoothManager::registrar{&descriptor};

BluetoothManager::BluetoothManager(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> object) : _obj(std::move(object)) {
    if (!_obj) throw std::runtime_error("Android BluetoothManager is unavailable.");
}

BluetoothGattServer BluetoothManager::openGattServer(const Context& context, Bridge::GattServerCallback& callback) {
    auto server = _obj.call_object_method(_open_gatt_server, context.get(), callback.get());
    if (!server) throw std::runtime_error("Android failed to open a Bluetooth GATT server.");
    return BluetoothGattServer(server.to_global());
}

}  // namespace SimpleBLE::Android
