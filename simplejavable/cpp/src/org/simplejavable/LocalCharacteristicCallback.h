#pragma once

#include <simpleble/Types.h>

#include <optional>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleJavaBLE {

/**
 * @brief C++ wrapper for the org.simplejavable.LocalCharacteristic$Callback Java interface.
 *
 * Holds a global reference, as the native characteristic outlives its Java object and keeps serving clients.
 */
class LocalCharacteristicCallback {
  public:
    explicit LocalCharacteristicCallback(jobject obj);

    /**
     * @brief Ask the Java read handler for a value.
     *
     * @return The value, or std::nullopt when Java returned null or the handler threw.
     */
    std::optional<::SimpleBLE::ByteArray> on_read() const noexcept;

    void on_write(const ::SimpleBLE::ByteArray& value) const;
    void on_subscribed() const;
    void on_unsubscribed() const;

  private:
    static jmethodID _method_on_read;
    static jmethodID _method_on_write;
    static jmethodID _method_on_subscribed;
    static jmethodID _method_on_unsubscribed;

    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<LocalCharacteristicCallback> registrar;

    SimpleJNI::Object<SimpleJNI::GlobalRef> _obj;
};

}  // namespace SimpleJavaBLE
}  // namespace Org
