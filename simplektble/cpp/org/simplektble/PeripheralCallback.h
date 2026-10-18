#pragma once

#include <optional>
#include <string>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleKtBLE {

class PeripheralCallback {
  public:
    PeripheralCallback() = default;
    explicit PeripheralCallback(jobject callback);

    void on_connected() const noexcept;
    void on_disconnected() const noexcept;

    // NOTE: The pairing calls run on the SimpleBLE thread that waits for the answer, and throw on Java exceptions.
    std::optional<std::string> on_passkey_request() const;
    void on_passkey_display(const std::string& passkey) const;
    bool on_numeric_comparison(const std::string& passkey) const;

  private:
    static jmethodID _method_on_connected;
    static jmethodID _method_on_disconnected;
    static jmethodID _method_on_passkey_request;
    static jmethodID _method_on_passkey_display;
    static jmethodID _method_on_numeric_comparison;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<PeripheralCallback> registrar;

    SimpleJNI::Object<SimpleJNI::WeakRef> _obj;
};

}  // namespace SimpleKtBLE
}  // namespace Org
