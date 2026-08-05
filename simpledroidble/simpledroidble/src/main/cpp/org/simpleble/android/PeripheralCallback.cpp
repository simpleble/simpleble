#include "PeripheralCallback.h"

#include "Callback.h"

namespace Org {
namespace SimpleBLE {
namespace Android {

jmethodID PeripheralCallback::_method_on_connected = nullptr;
jmethodID PeripheralCallback::_method_on_disconnected = nullptr;

const SimpleJNI::JNIDescriptor PeripheralCallback::descriptor{
    "org/simpleble/android/Peripheral$Callback",
    nullptr,
    {{"onConnected", "()V", &_method_on_connected}, {"onDisconnected", "()V", &_method_on_disconnected}}};

const SimpleJNI::AutoRegister<PeripheralCallback> PeripheralCallback::registrar{&descriptor};

PeripheralCallback::PeripheralCallback(jobject callback) : _obj(callback) {}

void PeripheralCallback::on_connected() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj.is_valid()) _obj.to_local().call_void_method(_method_on_connected);
    });
}

void PeripheralCallback::on_disconnected() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj.is_valid()) _obj.to_local().call_void_method(_method_on_disconnected);
    });
}

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
