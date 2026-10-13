#include "LocalPeripheralCallback.h"

namespace Org {
namespace SimpleJavaBLE {

jmethodID LocalPeripheralCallback::_method_on_client_connected = nullptr;
jmethodID LocalPeripheralCallback::_method_on_client_disconnected = nullptr;

const SimpleJNI::JNIDescriptor LocalPeripheralCallback::descriptor{
    "org/simplejavable/LocalPeripheral$Callback",  // Java interface name (inner class notation)
    nullptr,                                       // Callback wrappers only need the method IDs
    {{"onClientConnected", "(Ljava/lang/String;)V", &_method_on_client_connected},
     {"onClientDisconnected", "(Ljava/lang/String;)V", &_method_on_client_disconnected}}};

const SimpleJNI::AutoRegister<LocalPeripheralCallback> LocalPeripheralCallback::registrar{&descriptor};

LocalPeripheralCallback::LocalPeripheralCallback(jobject obj) : _obj(obj) {
    if (!_method_on_client_connected) {
        throw std::runtime_error("LocalPeripheralCallback JNI resources not preloaded");
    }
}

void LocalPeripheralCallback::on_client_connected(const std::string& address) const {
    SimpleJNI::String<SimpleJNI::LocalRef> value(address);
    _obj.call_void_method(_method_on_client_connected, value.get());
}

void LocalPeripheralCallback::on_client_disconnected(const std::string& address) const {
    SimpleJNI::String<SimpleJNI::LocalRef> value(address);
    _obj.call_void_method(_method_on_client_disconnected, value.get());
}

}  // namespace SimpleJavaBLE
}  // namespace Org
