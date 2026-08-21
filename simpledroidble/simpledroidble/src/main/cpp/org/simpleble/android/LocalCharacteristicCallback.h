#pragma once

#include <simpleble/Types.h>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org::SimpleBLE::Android {

class LocalCharacteristicCallback {
  public:
    explicit LocalCharacteristicCallback(jobject callback);
    ::SimpleBLE::ByteArray on_read() const;
    void on_write(const ::SimpleBLE::ByteArray& value) const noexcept;
    void on_subscribed() const noexcept;
    void on_unsubscribed() const noexcept;

  private:
    SimpleJNI::Object<SimpleJNI::WeakRef> _obj;
    static jmethodID _on_read;
    static jmethodID _on_write;
    static jmethodID _on_subscribed;
    static jmethodID _on_unsubscribed;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<LocalCharacteristicCallback> registrar;
};

}  // namespace Org::SimpleBLE::Android
