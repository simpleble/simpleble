#include "PeripheralCallback.h"

#include "Callback.h"
#include "simplejni/VM.hpp"

namespace Org {
namespace SimpleKtBLE {

jmethodID PeripheralCallback::_method_on_connected = nullptr;
jmethodID PeripheralCallback::_method_on_disconnected = nullptr;
jmethodID PeripheralCallback::_method_on_passkey_request = nullptr;
jmethodID PeripheralCallback::_method_on_passkey_display = nullptr;
jmethodID PeripheralCallback::_method_on_numeric_comparison = nullptr;

const SimpleJNI::JNIDescriptor PeripheralCallback::descriptor{
    "org/simplektble/PeripheralCallback",
    nullptr,
    {{"onConnected", "()V", &_method_on_connected},
     {"onDisconnected", "()V", &_method_on_disconnected},
     {"onPasskeyRequest", "()Ljava/lang/String;", &_method_on_passkey_request},
     {"onPasskeyDisplay", "(Ljava/lang/String;)V", &_method_on_passkey_display},
     {"onNumericComparison", "(Ljava/lang/String;)Z", &_method_on_numeric_comparison}}};

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

std::optional<std::string> PeripheralCallback::on_passkey_request() const {
    // The pairing calls run on SimpleBLE threads. Attach first, since other JNI calls attach as a non-daemon on
    // Android.
    SimpleJNI::VM::attach_as_daemon();

    // Returning nothing rejects the request, and SimpleBLE validates the six digits.
    if (!_obj.is_valid()) return std::nullopt;
    auto result = _obj.to_local().call_object_method(_method_on_passkey_request);
    if (!result) return std::nullopt;
    return SimpleJNI::String<SimpleJNI::LocalRef>(static_cast<jstring>(result.get())).str();
}

void PeripheralCallback::on_passkey_display(const std::string& passkey) const {
    SimpleJNI::VM::attach_as_daemon();
    if (!_obj.is_valid()) return;

    SimpleJNI::String<SimpleJNI::LocalRef> value(passkey);
    _obj.to_local().call_void_method(_method_on_passkey_display, value.get());
}

bool PeripheralCallback::on_numeric_comparison(const std::string& passkey) const {
    SimpleJNI::VM::attach_as_daemon();
    if (!_obj.is_valid()) return false;

    SimpleJNI::String<SimpleJNI::LocalRef> value(passkey);
    return _obj.to_local().call_boolean_method(_method_on_numeric_comparison, value.get());
}

}  // namespace SimpleKtBLE
}  // namespace Org
