#include "PeripheralCallback.h"

namespace Org {
namespace SimpleJavaBLE {

// Define static JNI resources
SimpleJNI::GlobalRef<jclass> PeripheralCallback::_cls;
jmethodID PeripheralCallback::_method_on_connected = nullptr;
jmethodID PeripheralCallback::_method_on_disconnected = nullptr;
jmethodID PeripheralCallback::_method_on_passkey_request = nullptr;
jmethodID PeripheralCallback::_method_on_passkey_display = nullptr;
jmethodID PeripheralCallback::_method_on_numeric_comparison = nullptr;

// Define the JNI descriptor
const SimpleJNI::JNIDescriptor PeripheralCallback::descriptor{
    "org/simplejavable/Peripheral$Callback",  // Java interface name (inner class notation)
    &_cls,                                    // Where to store the jclass
    {                                         // Methods to preload
     {"onConnected", "()V", &_method_on_connected},
     {"onDisconnected", "()V", &_method_on_disconnected},
     {"onPasskeyRequest", "()Ljava/lang/String;", &_method_on_passkey_request},
     {"onPasskeyDisplay", "(Ljava/lang/String;)V", &_method_on_passkey_display},
     {"onNumericComparison", "(Ljava/lang/String;)Z", &_method_on_numeric_comparison}}};

// Define the AutoRegister instance
const SimpleJNI::AutoRegister<PeripheralCallback> PeripheralCallback::registrar{&descriptor};

PeripheralCallback::PeripheralCallback(jobject obj) : _obj(obj) {
    if (!_cls.get()) {
        throw std::runtime_error("PeripheralCallback JNI resources not preloaded");
    }
}

void PeripheralCallback::on_connected() {
    if (_obj.is_valid()) {
        _obj.to_local().call_void_method(_method_on_connected);
    }
}

void PeripheralCallback::on_disconnected() {
    if (_obj.is_valid()) {
        _obj.to_local().call_void_method(_method_on_disconnected);
    }
}

std::optional<std::string> PeripheralCallback::on_passkey_request() {
    // Returning nothing rejects the request, and SimpleBLE validates the six digits.
    if (!_obj.is_valid()) return std::nullopt;

    auto result = _obj.to_local().call_object_method(_method_on_passkey_request);
    if (!result) return std::nullopt;
    return SimpleJNI::String<SimpleJNI::LocalRef>(static_cast<jstring>(result.get())).str();
}

void PeripheralCallback::on_passkey_display(const std::string& passkey) {
    if (_obj.is_valid()) {
        SimpleJNI::String<SimpleJNI::LocalRef> value(passkey);
        _obj.to_local().call_void_method(_method_on_passkey_display, value.get());
    }
}

bool PeripheralCallback::on_numeric_comparison(const std::string& passkey) {
    if (!_obj.is_valid()) return false;

    SimpleJNI::String<SimpleJNI::LocalRef> value(passkey);
    return _obj.to_local().call_boolean_method(_method_on_numeric_comparison, value.get());
}

}  // namespace SimpleJavaBLE
}  // namespace Org
