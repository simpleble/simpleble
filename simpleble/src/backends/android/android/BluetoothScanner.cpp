#include "BluetoothScanner.h"

namespace SimpleBLE {
namespace Android {

JNI::Class BluetoothScanner::_cls;
jmethodID BluetoothScanner::_method_startScan;
jmethodID BluetoothScanner::_method_stopScan;
jmethodID BluetoothScanner::_method_toString;

void BluetoothScanner::initialize() {
    JNI::Env env;

    if (_cls.get() == nullptr) {
        _cls = env.find_class("java/util/UUID");
    }

    if (!_method_toString) {
        _method_toString = env->GetMethodID(_cls.get(), "toString", "()Ljava/lang/String;");
    }

    if (!_method_startScan) {
        _method_startScan = env->GetMethodID(_cls.get(), "startScan", "(Landroid/bluetooth/le/ScanCallback;)V");
    }

    if (!_method_stopScan) {
        _method_stopScan = env->GetMethodID(_cls.get(), "stopScan", "(Landroid/bluetooth/le/ScanCallback;)V");
    }
}

BluetoothScanner::BluetoothScanner(JNI::Object obj) : _obj(obj) { initialize(); }

void BluetoothScanner::check_initialized() const {
    if (!_obj) throw std::runtime_error("BluetoothScanner is not initialized");
}

void BluetoothScanner::startScan(Bridge::ScanCallback &callback) {
    check_initialized();
    _obj.call_void_method(_method_startScan, "(Landroid/bluetooth/le/ScanCallback;)V", callback.get());
}

void BluetoothScanner::stopScan(Bridge::ScanCallback &callback) {
    check_initialized();
    _obj.call_void_method(_method_stopScan, "(Landroid/bluetooth/le/ScanCallback;)V", callback.get());
}

std::string BluetoothScanner::toString() {
    check_initialized();
    return _obj.call_string_method(_method_toString);
}

}  // namespace Android
}  // namespace SimpleBLE