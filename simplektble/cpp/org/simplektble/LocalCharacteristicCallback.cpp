#include "LocalCharacteristicCallback.h"

#include <string>

#include "Callback.h"
#include "Jvm.h"

namespace Org::SimpleKtBLE {

jmethodID LocalCharacteristicCallback::_on_read = nullptr;
jmethodID LocalCharacteristicCallback::_on_write = nullptr;
jmethodID LocalCharacteristicCallback::_on_subscribed = nullptr;
jmethodID LocalCharacteristicCallback::_on_unsubscribed = nullptr;
const SimpleJNI::JNIDescriptor LocalCharacteristicCallback::descriptor{
    "org/simplektble/LocalCharacteristicCallback",
    nullptr,
    {{"onRead", "()[B", &_on_read},
     {"onWrite", "([B)V", &_on_write},
     {"onSubscribed", "()V", &_on_subscribed},
     {"onUnsubscribed", "()V", &_on_unsubscribed}}};
const SimpleJNI::AutoRegister<LocalCharacteristicCallback> LocalCharacteristicCallback::registrar{&descriptor};

LocalCharacteristicCallback::LocalCharacteristicCallback(jobject callback) : _obj(callback) {}

std::optional<::SimpleBLE::ByteArray> LocalCharacteristicCallback::on_read() const noexcept {
    // NOTE: Failures are caught here instead of in SimpleBLE, so that every backend serves the stored value.
    try {
        // Runs on a SimpleBLE thread. Attach first, since any other JNI call would attach as a non-daemon.
        attach_as_daemon();
        if (!_obj) return std::nullopt;

        auto local = _obj.to_local();
        SimpleJNI::Env env;
        auto result = static_cast<jbyteArray>(env->CallObjectMethod(local.get(), _on_read));
        SimpleJNI::Exception::check(env);
        if (result == nullptr) return std::nullopt;
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> bytes(result);
        auto value = bytes.bytes();
        env->DeleteLocalRef(result);
        return value;
    } catch (const std::exception& exception) {
        ::SimpleBLE::Logging::Logger::get()->log(::SimpleBLE::Logging::Level::Error, "SimpleKtBLE", __FILE__,
                                                 __LINE__, __func__,
                                                 std::string("Kotlin read handler failed: ") + exception.what());
        return std::nullopt;
    }
}

void LocalCharacteristicCallback::on_write(const ::SimpleBLE::ByteArray& value) const noexcept {
    invoke_callback(__func__, [this, &value] {
        if (!_obj) return;
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> bytes(value);
        _obj.to_local().call_void_method(_on_write, bytes.get());
    });
}

void LocalCharacteristicCallback::on_subscribed() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj) _obj.to_local().call_void_method(_on_subscribed);
    });
}

void LocalCharacteristicCallback::on_unsubscribed() const noexcept {
    invoke_callback(__func__, [this] {
        if (_obj) _obj.to_local().call_void_method(_on_unsubscribed);
    });
}

}  // namespace Org::SimpleKtBLE
