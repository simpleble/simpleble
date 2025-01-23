#include "BluetoothAdapter.h"

namespace SimpleBLE {
namespace Android {

JNI::Class BluetoothAdapter::_cls;
jmethodID BluetoothAdapter::_method_getName;
jmethodID BluetoothAdapter::_method_getAddress;
jmethodID BluetoothAdapter::_method_isEnabled;
jmethodID BluetoothAdapter::_method_getState;
jmethodID BluetoothAdapter::_method_getBluetoothLeScanner;
jmethodID BluetoothAdapter::_method_getDefaultAdapter;

void BluetoothAdapter::initialize() {
    JNI::Env env;

    if (_cls.get() == nullptr) {
        _cls = env.find_class("android/bluetooth/BluetoothAdapter");
    }

    if (_method_getName == nullptr) {
        _method_getName = env->GetMethodID(_cls.get(), "getName", "()Ljava/lang/String;");
    }

    if (_method_getAddress == nullptr) {
        _method_getAddress = env->GetMethodID(_cls.get(), "getAddress", "()Ljava/lang/String;");
    }

    if (_method_isEnabled == nullptr) {
        _method_isEnabled = env->GetMethodID(_cls.get(), "isEnabled", "()Z");
    }

    if (_method_getState == nullptr) {
        _method_getState = env->GetMethodID(_cls.get(), "getState", "()I");
    }

    if (_method_getBluetoothLeScanner == nullptr) {
        _method_getBluetoothLeScanner = env->GetMethodID(_cls.get(), "getBluetoothLeScanner", "()Landroid/bluetooth/le/BluetoothLeScanner;");
    }

    if (_method_getDefaultAdapter == nullptr) {
        _method_getDefaultAdapter = env->GetStaticMethodID(_cls.get(), "getDefaultAdapter", "()Landroid/bluetooth/BluetoothAdapter;");
    }
}

BluetoothAdapter::BluetoothAdapter(JNI::Object obj) : _obj(obj) {
    initialize();
}

void BluetoothAdapter::check_initialized() const {
    if (!_obj) throw std::runtime_error("BluetoothAdapter is not initialized");
}

BluetoothAdapter BluetoothAdapter::getDefaultAdapter() {
    initialize();
    return BluetoothAdapter(_cls.call_static_method(_method_getDefaultAdapter));
}

std::string BluetoothAdapter::getName() {
    check_initialized();
    return _obj.call_string_method(_method_getName);
}

std::string BluetoothAdapter::getAddress() {
    check_initialized();
    return _obj.call_string_method(_method_getAddress);
}

bool BluetoothAdapter::isEnabled() {
    check_initialized();
    return _obj.call_boolean_method(_method_isEnabled);
}

int BluetoothAdapter::getState() {
    check_initialized();
    return _obj.call_int_method(_method_getState);
}

BluetoothScanner BluetoothAdapter::getBluetoothLeScanner() {
    check_initialized();
    return BluetoothScanner(_obj.call_object_method(_method_getBluetoothLeScanner));
}

}  // namespace Android
}  // namespace SimpleBLE
