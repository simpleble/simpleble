#pragma once
#include <optional>
#include <string>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleJavaBLE {

/**
 * @brief C++ wrapper for the org.simplejavable.Peripheral$Callback Java interface.
 *
 * This class encapsulates a jobject representing an instance of the Callback interface
 * from the org.simplejavable.Peripheral class. It provides methods to invoke the interface's
 * Java methods (onConnected, onDisconnected and the Dongl pairing callbacks) via JNI.
 * JNI resources are preloaded during JNI_OnLoad using the AutoRegister mechanism.
 */
class PeripheralCallback {
  public:
    PeripheralCallback() = default;

    /**
     * @brief Construct a Callback from a Java object.
     *
     * @param obj The Java object implementing the Callback interface.
     * @throws std::runtime_error if JNI resources are not preloaded.
     */
    explicit PeripheralCallback(jobject obj);

    /**
     * @brief Invoke the onConnected method on the Java Callback object.
     */
    void on_connected();

    /**
     * @brief Invoke the onDisconnected method on the Java Callback object.
     */
    void on_disconnected();

    std::optional<std::string> on_passkey_request();
    void on_passkey_display(const std::string& passkey);
    bool on_numeric_comparison(const std::string& passkey);

  private:
    // Static JNI resources, populated by Registrar during JNI_OnLoad
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _method_on_connected;
    static jmethodID _method_on_disconnected;
    static jmethodID _method_on_passkey_request;
    static jmethodID _method_on_passkey_display;
    static jmethodID _method_on_numeric_comparison;

    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<PeripheralCallback> registrar;

    SimpleJNI::Object<SimpleJNI::WeakRef> _obj;
};

}  // namespace SimpleJavaBLE
}  // namespace Org