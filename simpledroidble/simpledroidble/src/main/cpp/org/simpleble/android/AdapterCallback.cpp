#include "AdapterCallback.h"

#include "Callback.h"

namespace Org {
namespace SimpleBLE {
namespace Android {

jmethodID AdapterCallback::_method_on_scan_start = nullptr;
jmethodID AdapterCallback::_method_on_scan_stop = nullptr;
jmethodID AdapterCallback::_method_on_scan_updated = nullptr;
jmethodID AdapterCallback::_method_on_scan_found = nullptr;

const SimpleJNI::JNIDescriptor AdapterCallback::descriptor{
    "org/simpleble/android/Adapter$Callback",
    nullptr,
    {{"onScanStart", "()V", &_method_on_scan_start},
     {"onScanStop", "()V", &_method_on_scan_stop},
     {"onScanUpdated", "(J)V", &_method_on_scan_updated},
     {"onScanFound", "(J)V", &_method_on_scan_found}}};

const SimpleJNI::AutoRegister<AdapterCallback> AdapterCallback::registrar{&descriptor};

AdapterCallback::AdapterCallback(jobject callback) : _obj(callback) {}

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

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
