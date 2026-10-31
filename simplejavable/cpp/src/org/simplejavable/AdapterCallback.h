#pragma once
#include <optional>
#include <string>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleJavaBLE {

/**
 * @brief C++ wrapper for the org.simplejavable.Adapter$Callback Java interface.
 *
 * This class encapsulates a jobject representing an instance of the Callback interface
 * from the org.simplejavable.Adapter class. It provides methods to invoke the interface's
 * Java methods (onPowerOn, onPowerOff, onScanStart, onScanStop, onScanUpdated, onScanFound and
 * the Linux pairing callbacks) via JNI. JNI resources are preloaded during JNI_OnLoad using the
 * AutoRegister mechanism.
 */
class AdapterCallback {
  public:
    AdapterCallback() = default;

    /**
     * @brief Construct a Callback from a Java object.
     *
     * Initializes the internal SimpleJNI::Object with the provided jobject and the preloaded
     * jclass. Throws if JNI resources aren’t preloaded, enforcing initialization via JNI_OnLoad.
     * @param obj The Java object implementing the Callback interface.
     * @throws std::runtime_error if JNI resources are not preloaded.
     */
    explicit AdapterCallback(jobject obj);

    /**
     * @brief Invoke the onPowerOn method on the Java Callback object.
     *
     * Calls the Java onPowerOn() method, which has no arguments and returns void.
     */
    void on_power_on();

    /**
     * @brief Invoke the onPowerOff method on the Java Callback object.
     *
     * Calls the Java onPowerOff() method, which has no arguments and returns void.
     */
    void on_power_off();

    /**
     * @brief Invoke the onScanStart method on the Java Callback object.
     *
     * Calls the Java onScanStart() method, which has no arguments and returns void.
     */
    void on_scan_start();

    /**
     * @brief Invoke the onScanStop method on the Java Callback object.
     *
     * Calls the Java onScanStop() method, which has no arguments and returns void.
     */
    void on_scan_stop();

    /**
     * @brief Invoke the onScanUpdated method on the Java Callback object.
     *
     * Calls the Java onScanUpdated(long) method, passing a peripheral ID as a jlong.
     * @param peripheral_id The ID of the peripheral that was updated.
     */
    void on_scan_updated(int64_t peripheral_id);

    /**
     * @brief Invoke the onScanFound method on the Java Callback object.
     *
     * Calls the Java onScanFound(long) method, passing a peripheral ID as a jlong.
     * @param peripheral_id The ID of the peripheral that was found.
     */
    void on_scan_found(int64_t peripheral_id);

    std::optional<std::string> on_passkey_request(const std::string& peer);
    void on_passkey_display(const std::string& peer, const std::string& passkey);
    bool on_numeric_comparison(const std::string& peer, const std::string& passkey);
    bool on_pairing_authorization(const std::string& peer);

  private:
    // Static JNI resources, populated by Registrar during JNI_OnLoad
    static SimpleJNI::GlobalRef<jclass> _cls;  ///< Preloaded jclass for org.simplejavable.Adapter$Callback.
    static jmethodID _method_on_power_on;      ///< Preloaded jmethodID for onPowerOn.
    static jmethodID _method_on_power_off;     ///< Preloaded jmethodID for onPowerOff.
    static jmethodID _method_on_scan_start;    ///< Preloaded jmethodID for onScanStart.
    static jmethodID _method_on_scan_stop;     ///< Preloaded jmethodID for onScanStop.
    static jmethodID _method_on_scan_updated;  ///< Preloaded jmethodID for onScanUpdated.
    static jmethodID _method_on_scan_found;    ///< Preloaded jmethodID for onScanFound.
    static jmethodID _method_on_passkey_request;
    static jmethodID _method_on_passkey_display;
    static jmethodID _method_on_numeric_comparison;
    static jmethodID _method_on_pairing_authorization;

    /**
     * @brief JNI descriptor defining the Callback interface and its methods.
     *
     * Declares the Java interface and its methods, linking to static storage locations
     * where Registrar stores resolved JNI resources. Defined in Callback.cpp to ensure
     * a single point of definition.
     */
    static const SimpleJNI::JNIDescriptor descriptor;

    /**
     * @brief Automatic registration of the descriptor with Registrar.
     *
     * Triggers registration during static initialization of Callback.cpp, ensuring the
     * descriptor is added to Registrar before JNI_OnLoad. Defined in Callback.cpp to
     * maintain a single instantiation.
     */
    static const SimpleJNI::AutoRegister<AdapterCallback> registrar;

    SimpleJNI::Object<SimpleJNI::WeakRef> _obj;  ///< Wrapped Java object with WeakGlobalRef lifetime.
};

}  // namespace SimpleJavaBLE
}  // namespace Org