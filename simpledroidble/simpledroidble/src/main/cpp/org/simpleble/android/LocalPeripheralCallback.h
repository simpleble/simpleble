#pragma once

#include <string>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org::SimpleBLE::Android {

class LocalPeripheralCallback {
  public:
    explicit LocalPeripheralCallback(jobject callback);
    void on_client_connected(const std::string& address) const noexcept;
    void on_client_disconnected(const std::string& address) const noexcept;

  private:
    SimpleJNI::Object<SimpleJNI::WeakRef> _obj;
    static jmethodID _on_client_connected;
    static jmethodID _on_client_disconnected;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<LocalPeripheralCallback> registrar;
};

}  // namespace Org::SimpleBLE::Android
