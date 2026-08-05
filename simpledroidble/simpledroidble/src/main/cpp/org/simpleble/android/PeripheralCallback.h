#pragma once

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleBLE {
namespace Android {

class PeripheralCallback {
  public:
    PeripheralCallback() = default;
    explicit PeripheralCallback(jobject callback);

    void on_connected() const noexcept;
    void on_disconnected() const noexcept;

  private:
    static jmethodID _method_on_connected;
    static jmethodID _method_on_disconnected;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<PeripheralCallback> registrar;

    SimpleJNI::Object<SimpleJNI::WeakRef> _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
