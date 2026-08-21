#include "LocalPeripheralCallback.h"

#include "Callback.h"

namespace Org::SimpleBLE::Android {

jmethodID LocalPeripheralCallback::_on_client_connected = nullptr;
jmethodID LocalPeripheralCallback::_on_client_disconnected = nullptr;
const SimpleJNI::JNIDescriptor LocalPeripheralCallback::descriptor{
    "org/simpleble/android/LocalPeripheral$Callback",
    nullptr,
    {{"onClientConnected", "(Ljava/lang/String;)V", &_on_client_connected},
     {"onClientDisconnected", "(Ljava/lang/String;)V", &_on_client_disconnected}}};
const SimpleJNI::AutoRegister<LocalPeripheralCallback> LocalPeripheralCallback::registrar{&descriptor};

LocalPeripheralCallback::LocalPeripheralCallback(jobject callback) : _obj(callback) {}

void LocalPeripheralCallback::on_client_connected(const std::string& address) const noexcept {
    invoke_callback(__func__, [this, &address] {
        if (!_obj.is_valid()) return;
        SimpleJNI::String<SimpleJNI::LocalRef> value(address);
        _obj.to_local().call_void_method(_on_client_connected, value.get());
    });
}

void LocalPeripheralCallback::on_client_disconnected(const std::string& address) const noexcept {
    invoke_callback(__func__, [this, &address] {
        if (!_obj.is_valid()) return;
        SimpleJNI::String<SimpleJNI::LocalRef> value(address);
        _obj.to_local().call_void_method(_on_client_disconnected, value.get());
    });
}

}  // namespace Org::SimpleBLE::Android
