#include "LocalCharacteristicCallback.h"

#include <simpleble/Logging.h>

#include <string>

namespace Org {
namespace SimpleJavaBLE {

jmethodID LocalCharacteristicCallback::_method_on_read = nullptr;
jmethodID LocalCharacteristicCallback::_method_on_write = nullptr;
jmethodID LocalCharacteristicCallback::_method_on_subscribed = nullptr;
jmethodID LocalCharacteristicCallback::_method_on_unsubscribed = nullptr;

const SimpleJNI::JNIDescriptor LocalCharacteristicCallback::descriptor{
    "org/simplejavable/LocalCharacteristic$Callback",  // Java interface name (inner class notation)
    nullptr,                                           // Callback wrappers only need the method IDs
    {{"onRead", "()[B", &_method_on_read},
     {"onWrite", "([B)V", &_method_on_write},
     {"onSubscribed", "()V", &_method_on_subscribed},
     {"onUnsubscribed", "()V", &_method_on_unsubscribed}}};

const SimpleJNI::AutoRegister<LocalCharacteristicCallback> LocalCharacteristicCallback::registrar{&descriptor};

LocalCharacteristicCallback::LocalCharacteristicCallback(jobject obj) : _obj(obj) {
    if (!_method_on_read) {
        throw std::runtime_error("LocalCharacteristicCallback JNI resources not preloaded");
    }
}

std::optional<::SimpleBLE::ByteArray> LocalCharacteristicCallback::on_read() const noexcept {
    // NOTE: We catch here instead of leaving it to SimpleBLE, so that a failed handler serves the stored value
    // on every backend.
    try {
        auto result = _obj.call_object_method(_method_on_read);
        if (!result) return std::nullopt;
        return SimpleJNI::ByteArray<SimpleJNI::LocalRef>(result.get()).bytes();
    } catch (const std::exception& exception) {
        ::SimpleBLE::Logging::Logger::get()->log(::SimpleBLE::Logging::Level::Error, "SimpleJavaBLE", __FILE__,
                                                 __LINE__, __func__,
                                                 std::string("Java read handler failed: ") + exception.what());
        return std::nullopt;
    }
}

void LocalCharacteristicCallback::on_write(const ::SimpleBLE::ByteArray& value) const {
    SimpleJNI::ByteArray<SimpleJNI::LocalRef> bytes(value);
    _obj.call_void_method(_method_on_write, bytes.get());
}

void LocalCharacteristicCallback::on_subscribed() const { _obj.call_void_method(_method_on_subscribed); }

void LocalCharacteristicCallback::on_unsubscribed() const { _obj.call_void_method(_method_on_unsubscribed); }

}  // namespace SimpleJavaBLE
}  // namespace Org
