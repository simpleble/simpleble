#include "LocalCharacteristicCallback.h"

#include <stdexcept>

#include "Callback.h"

namespace Org::SimpleBLE::Android {

jmethodID LocalCharacteristicCallback::_on_read = nullptr;
jmethodID LocalCharacteristicCallback::_on_write = nullptr;
jmethodID LocalCharacteristicCallback::_on_subscribed = nullptr;
jmethodID LocalCharacteristicCallback::_on_unsubscribed = nullptr;
const SimpleJNI::JNIDescriptor LocalCharacteristicCallback::descriptor{
    "org/simpleble/android/LocalCharacteristic$Callback",
    nullptr,
    {{"onRead", "()[B", &_on_read},
     {"onWrite", "([B)V", &_on_write},
     {"onSubscribed", "()V", &_on_subscribed},
     {"onUnsubscribed", "()V", &_on_unsubscribed}}};
const SimpleJNI::AutoRegister<LocalCharacteristicCallback> LocalCharacteristicCallback::registrar{&descriptor};

LocalCharacteristicCallback::LocalCharacteristicCallback(jobject callback) : _obj(callback) {}

::SimpleBLE::ByteArray LocalCharacteristicCallback::on_read() const {
    if (!_obj.is_valid()) throw std::runtime_error("The local characteristic read handler is no longer available.");
    auto local = _obj.to_local();
    SimpleJNI::Env env;
    auto result = static_cast<jbyteArray>(env->CallObjectMethod(local.get(), _on_read));
    SimpleJNI::Exception::check(env);
    if (result == nullptr) throw std::runtime_error("A local characteristic read handler returned null.");
    SimpleJNI::ByteArray<SimpleJNI::LocalRef> bytes(result);
    auto value = bytes.bytes();
    env->DeleteLocalRef(result);
    return value;
}

void LocalCharacteristicCallback::on_write(const ::SimpleBLE::ByteArray& value) const noexcept {
    invoke_callback(__func__, [this, &value] {
        if (!_obj.is_valid()) return;
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> bytes(value);
        _obj.to_local().call_void_method(_on_write, bytes.get());
    });
}

void LocalCharacteristicCallback::on_subscribed() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj.is_valid()) _obj.to_local().call_void_method(_on_subscribed);
    });
}

void LocalCharacteristicCallback::on_unsubscribed() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj.is_valid()) _obj.to_local().call_void_method(_on_unsubscribed);
    });
}

}  // namespace Org::SimpleBLE::Android
