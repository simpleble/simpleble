#pragma once

#include <string>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org::SimpleKtBLE {

class LocalPeripheralCallback {
  public:
    explicit LocalPeripheralCallback(jobject callback);
    void on_client_connected(const std::string& address) const noexcept;
    void on_client_disconnected(const std::string& address) const noexcept;

  private:
    // NOTE: A strong reference, since nothing else keeps the Kotlin object alive while the peripheral is hosted.
    SimpleJNI::Object<SimpleJNI::GlobalRef> _obj;
    static jmethodID _on_client_connected;
    static jmethodID _on_client_disconnected;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<LocalPeripheralCallback> registrar;
};

}  // namespace Org::SimpleKtBLE
