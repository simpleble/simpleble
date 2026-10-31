#include "AdapterCallback.h"

#include "Callback.h"
#include "simplejni/VM.hpp"

namespace Org {
namespace SimpleKtBLE {

jmethodID AdapterCallback::_method_on_power_on = nullptr;
jmethodID AdapterCallback::_method_on_power_off = nullptr;
jmethodID AdapterCallback::_method_on_scan_start = nullptr;
jmethodID AdapterCallback::_method_on_scan_stop = nullptr;
jmethodID AdapterCallback::_method_on_scan_updated = nullptr;
jmethodID AdapterCallback::_method_on_scan_found = nullptr;
jmethodID AdapterCallback::_method_on_passkey_request = nullptr;
jmethodID AdapterCallback::_method_on_passkey_display = nullptr;
jmethodID AdapterCallback::_method_on_numeric_comparison = nullptr;
jmethodID AdapterCallback::_method_on_pairing_authorization = nullptr;

const SimpleJNI::JNIDescriptor AdapterCallback::descriptor{
    "org/simplektble/AdapterCallback",
    nullptr,
    {{"onPowerOn", "()V", &_method_on_power_on},
     {"onPowerOff", "()V", &_method_on_power_off},
     {"onScanStart", "()V", &_method_on_scan_start},
     {"onScanStop", "()V", &_method_on_scan_stop},
     {"onScanUpdated", "(J)V", &_method_on_scan_updated},
     {"onScanFound", "(J)V", &_method_on_scan_found},
     {"onPasskeyRequest", "(Ljava/lang/String;)Ljava/lang/String;", &_method_on_passkey_request},
     {"onPasskeyDisplay", "(Ljava/lang/String;Ljava/lang/String;)V", &_method_on_passkey_display},
     {"onNumericComparison", "(Ljava/lang/String;Ljava/lang/String;)Z", &_method_on_numeric_comparison},
     {"onPairingAuthorization", "(Ljava/lang/String;)Z", &_method_on_pairing_authorization}}};

const SimpleJNI::AutoRegister<AdapterCallback> AdapterCallback::registrar{&descriptor};

AdapterCallback::AdapterCallback(jobject callback) : _obj(callback) {}

void AdapterCallback::on_power_on() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj.is_valid()) _obj.to_local().call_void_method(_method_on_power_on);
    });
}

void AdapterCallback::on_power_off() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj.is_valid()) _obj.to_local().call_void_method(_method_on_power_off);
    });
}

void AdapterCallback::on_scan_start() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj.is_valid()) _obj.to_local().call_void_method(_method_on_scan_start);
    });
}

void AdapterCallback::on_scan_stop() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj.is_valid()) _obj.to_local().call_void_method(_method_on_scan_stop);
    });
}

void AdapterCallback::on_scan_updated(int64_t peripheral_id) const noexcept {
    invoke_callback(__func__, [this, peripheral_id] {
        if (_obj.is_valid()) {
            _obj.to_local().call_void_method(_method_on_scan_updated, static_cast<jlong>(peripheral_id));
        }
    });
}

void AdapterCallback::on_scan_found(int64_t peripheral_id) const noexcept {
    invoke_callback(__func__, [this, peripheral_id] {
        if (_obj.is_valid()) {
            _obj.to_local().call_void_method(_method_on_scan_found, static_cast<jlong>(peripheral_id));
        }
    });
}

std::optional<std::string> AdapterCallback::on_passkey_request(const std::string& peer) const {
    // The pairing calls run on SimpleBLE's BlueZ thread, which attaches as a daemon so the JVM can still exit.
    SimpleJNI::VM::attach_as_daemon();

    // Returning nothing rejects the request, and SimpleBLE validates the six digits.
    if (!_obj.is_valid()) return std::nullopt;
    SimpleJNI::String<SimpleJNI::LocalRef> address(peer);
    auto result = _obj.to_local().call_object_method(_method_on_passkey_request, address.get());
    if (!result) return std::nullopt;
    return SimpleJNI::String<SimpleJNI::LocalRef>(static_cast<jstring>(result.get())).str();
}

void AdapterCallback::on_passkey_display(const std::string& peer, const std::string& passkey) const {
    SimpleJNI::VM::attach_as_daemon();
    if (!_obj.is_valid()) return;

    SimpleJNI::String<SimpleJNI::LocalRef> address(peer);
    SimpleJNI::String<SimpleJNI::LocalRef> value(passkey);
    _obj.to_local().call_void_method(_method_on_passkey_display, address.get(), value.get());
}

bool AdapterCallback::on_numeric_comparison(const std::string& peer, const std::string& passkey) const {
    SimpleJNI::VM::attach_as_daemon();
    if (!_obj.is_valid()) return false;

    SimpleJNI::String<SimpleJNI::LocalRef> address(peer);
    SimpleJNI::String<SimpleJNI::LocalRef> value(passkey);
    return _obj.to_local().call_boolean_method(_method_on_numeric_comparison, address.get(), value.get());
}

bool AdapterCallback::on_pairing_authorization(const std::string& peer) const {
    SimpleJNI::VM::attach_as_daemon();
    // NOTE: Without a handler, Just Works pairing is accepted, as SimpleBLE does without a callback.
    if (!_obj.is_valid()) return true;

    SimpleJNI::String<SimpleJNI::LocalRef> address(peer);
    return _obj.to_local().call_boolean_method(_method_on_pairing_authorization, address.get());
}

}  // namespace SimpleKtBLE
}  // namespace Org
