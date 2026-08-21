#include "BluetoothGattServer.h"

#include <utility>

namespace SimpleBLE::Android {

SimpleJNI::GlobalRef<jclass> BluetoothGattServer::_cls;
jmethodID BluetoothGattServer::_add_service = nullptr;
jmethodID BluetoothGattServer::_clear_services = nullptr;
jmethodID BluetoothGattServer::_close = nullptr;
jmethodID BluetoothGattServer::_send_response = nullptr;
jmethodID BluetoothGattServer::_notify_characteristic_changed = nullptr;

const SimpleJNI::JNIDescriptor BluetoothGattServer::descriptor{
    "android/bluetooth/BluetoothGattServer",
    &_cls,
    {{"addService", "(Landroid/bluetooth/BluetoothGattService;)Z", &_add_service},
     {"clearServices", "()V", &_clear_services},
     {"close", "()V", &_close},
     {"sendResponse", "(Landroid/bluetooth/BluetoothDevice;III[B)Z", &_send_response},
     {"notifyCharacteristicChanged",
      "(Landroid/bluetooth/BluetoothDevice;Landroid/bluetooth/BluetoothGattCharacteristic;Z)Z",
      &_notify_characteristic_changed}}};
const SimpleJNI::AutoRegister<BluetoothGattServer> BluetoothGattServer::registrar{&descriptor};

BluetoothGattServer::BluetoothGattServer(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> object)
    : _obj(std::move(object)) {}

bool BluetoothGattServer::addService(const BluetoothGattService& service) {
    return _obj.call_boolean_method(_add_service, service.getObject().get());
}

void BluetoothGattServer::clearServices() { _obj.call_void_method(_clear_services); }

void BluetoothGattServer::close() {
    if (_obj) _obj.call_void_method(_close);
    _obj = {};
}

bool BluetoothGattServer::sendResponse(const BluetoothDevice& device, int request_id, int status, int offset,
                                       const std::vector<uint8_t>& value) {
    SimpleJNI::ByteArray<SimpleJNI::LocalRef> bytes(value);
    return _obj.call_boolean_method(_send_response, device.get(), request_id, status, offset, bytes.get());
}

bool BluetoothGattServer::notifyCharacteristicChanged(const BluetoothDevice& device,
                                                      const BluetoothGattCharacteristic& characteristic, bool confirm) {
    return _obj.call_boolean_method(_notify_characteristic_changed, device.get(), characteristic.getObject().get(),
                                    confirm);
}

}  // namespace SimpleBLE::Android
