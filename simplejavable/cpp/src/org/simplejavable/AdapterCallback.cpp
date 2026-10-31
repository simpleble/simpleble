#include "AdapterCallback.h"

namespace Org {
namespace SimpleJavaBLE {

// Define static JNI resources
SimpleJNI::GlobalRef<jclass> AdapterCallback::_cls;
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

// Define the JNI descriptor
const SimpleJNI::JNIDescriptor AdapterCallback::descriptor{
    "org/simplejavable/Adapter$Callback",  // Java interface name (inner class notation)
    &_cls,                                 // Where to store the jclass
    {                                      // Methods to preload
     {"onPowerOn", "()V", &_method_on_power_on},
     {"onPowerOff", "()V", &_method_on_power_off},
     {"onScanStart", "()V", &_method_on_scan_start},
     {"onScanStop", "()V", &_method_on_scan_stop},
     {"onScanUpdated", "(J)V", &_method_on_scan_updated},
     {"onScanFound", "(J)V", &_method_on_scan_found},
     {"onPasskeyRequest", "(Ljava/lang/String;)Ljava/lang/String;", &_method_on_passkey_request},
     {"onPasskeyDisplay", "(Ljava/lang/String;Ljava/lang/String;)V", &_method_on_passkey_display},
     {"onNumericComparison", "(Ljava/lang/String;Ljava/lang/String;)Z", &_method_on_numeric_comparison},
     {"onPairingAuthorization", "(Ljava/lang/String;)Z", &_method_on_pairing_authorization}}};

// Define the AutoRegister instance
const SimpleJNI::AutoRegister<AdapterCallback> AdapterCallback::registrar{&descriptor};

/**
 * @brief Construct a Callback from a Java object.
 *
 * Initializes the internal SimpleJNI::Object with the provided jobject. The check for
 * _cls.get() ensures that JNI_OnLoad has preloaded the callback method IDs.
 * @param obj The Java object implementing the Callback interface.
 * @throws std::runtime_error if JNI resources are not preloaded.
 */
AdapterCallback::AdapterCallback(jobject obj) : _obj(obj) {
    if (!_cls.get()) {
        throw std::runtime_error("AdapterCallback JNI resources not preloaded");
    }
}

/**
 * @brief Invoke the onPowerOn method on the Java Callback object.
 *
 * Calls the Java onPowerOn() method using the preloaded jmethodID.
 */
void AdapterCallback::on_power_on() {
    if (_obj.is_valid()) {
        _obj.to_local().call_void_method(_method_on_power_on);
    }
}

/**
 * @brief Invoke the onPowerOff method on the Java Callback object.
 *
 * Calls the Java onPowerOff() method using the preloaded jmethodID.
 */
void AdapterCallback::on_power_off() {
    if (_obj.is_valid()) {
        _obj.to_local().call_void_method(_method_on_power_off);
    }
}

/**
 * @brief Invoke the onScanStart method on the Java Callback object.
 *
 * Uses the preloaded _method_on_scan_start to call the Java method via SimpleJNI::Object.
 * No additional checks are needed here since the constructor enforces resource availability.
 */
void AdapterCallback::on_scan_start() {
    if (_obj.is_valid()) {
        _obj.to_local().call_void_method(_method_on_scan_start);
    }
}

/**
 * @brief Invoke the onScanStop method on the Java Callback object.
 *
 * Calls the Java onScanStop() method using the preloaded jmethodID.
 */
void AdapterCallback::on_scan_stop() {
    if (_obj.is_valid()) {
        _obj.to_local().call_void_method(_method_on_scan_stop);
    }
}

/**
 * @brief Invoke the onScanUpdated method on the Java Callback object.
 *
 * Calls the Java onScanUpdated(long) method, converting the C++ int64_t to a JNI jlong.
 * The preloaded jmethodID ensures the call is efficient and safe.
 * @param peripheral_id The ID of the peripheral that was updated.
 */
void AdapterCallback::on_scan_updated(int64_t peripheral_id) {
    if (_obj.is_valid()) {
        _obj.to_local().call_void_method(_method_on_scan_updated, static_cast<jlong>(peripheral_id));
    }
}

/**
 * @brief Invoke the onScanFound method on the Java Callback object.
 *
 * Calls the Java onScanFound(long) method with the provided peripheral ID, using the
 * preloaded jmethodID for efficiency.
 * @param peripheral_id The ID of the peripheral that was found.
 */
void AdapterCallback::on_scan_found(int64_t peripheral_id) {
    if (_obj.is_valid()) {
        _obj.to_local().call_void_method(_method_on_scan_found, static_cast<jlong>(peripheral_id));
    }
}

std::optional<std::string> AdapterCallback::on_passkey_request(const std::string& peer) {
    // Returning nothing rejects the request, and SimpleBLE validates the six digits.
    if (!_obj.is_valid()) return std::nullopt;

    SimpleJNI::String<SimpleJNI::LocalRef> address(peer);
    auto result = _obj.to_local().call_object_method(_method_on_passkey_request, address.get());
    if (!result) return std::nullopt;
    return SimpleJNI::String<SimpleJNI::LocalRef>(static_cast<jstring>(result.get())).str();
}

void AdapterCallback::on_passkey_display(const std::string& peer, const std::string& passkey) {
    if (_obj.is_valid()) {
        SimpleJNI::String<SimpleJNI::LocalRef> address(peer);
        SimpleJNI::String<SimpleJNI::LocalRef> value(passkey);
        _obj.to_local().call_void_method(_method_on_passkey_display, address.get(), value.get());
    }
}

bool AdapterCallback::on_numeric_comparison(const std::string& peer, const std::string& passkey) {
    if (!_obj.is_valid()) return false;

    SimpleJNI::String<SimpleJNI::LocalRef> address(peer);
    SimpleJNI::String<SimpleJNI::LocalRef> value(passkey);
    return _obj.to_local().call_boolean_method(_method_on_numeric_comparison, address.get(), value.get());
}

bool AdapterCallback::on_pairing_authorization(const std::string& peer) {
    if (!_obj.is_valid()) return false;

    SimpleJNI::String<SimpleJNI::LocalRef> address(peer);
    return _obj.to_local().call_boolean_method(_method_on_pairing_authorization, address.get());
}

}  // namespace SimpleJavaBLE
}  // namespace Org
