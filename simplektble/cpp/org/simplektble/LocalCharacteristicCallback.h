#pragma once

#include <simpleble/Types.h>

#include <optional>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org::SimpleKtBLE {

class LocalCharacteristicCallback {
  public:
    explicit LocalCharacteristicCallback(jobject callback);
    /** Returns nothing when the Kotlin handler fails or is not set, so the stored value is served instead. */
    std::optional<::SimpleBLE::ByteArray> on_read() const noexcept;
    void on_write(const ::SimpleBLE::ByteArray& value) const noexcept;
    void on_subscribed() const noexcept;
    void on_unsubscribed() const noexcept;

  private:
    // NOTE: A strong reference, since nothing else keeps the Kotlin object alive while the peripheral is hosted.
    SimpleJNI::Object<SimpleJNI::GlobalRef> _obj;
    static jmethodID _on_read;
    static jmethodID _on_write;
    static jmethodID _on_subscribed;
    static jmethodID _on_unsubscribed;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<LocalCharacteristicCallback> registrar;
};

}  // namespace Org::SimpleKtBLE
