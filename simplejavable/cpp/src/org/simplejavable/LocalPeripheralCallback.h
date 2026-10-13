#pragma once

#include <string>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleJavaBLE {

/**
 * @brief C++ wrapper for the org.simplejavable.LocalPeripheral$Callback Java interface.
 *
 * Holds a global reference: the native host outlives its Java wrapper and keeps serving clients.
 */
class LocalPeripheralCallback {
  public:
    explicit LocalPeripheralCallback(jobject obj);

    void on_client_connected(const std::string& address) const;
    void on_client_disconnected(const std::string& address) const;

  private:
    static jmethodID _method_on_client_connected;
    static jmethodID _method_on_client_disconnected;

    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<LocalPeripheralCallback> registrar;

    SimpleJNI::Object<SimpleJNI::GlobalRef> _obj;
};

}  // namespace SimpleJavaBLE
}  // namespace Org
