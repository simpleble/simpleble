#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleKtBLE {

class AdapterCallback {
  public:
    AdapterCallback() = default;
    explicit AdapterCallback(jobject callback);

    void on_power_on() const noexcept;
    void on_power_off() const noexcept;
    void on_scan_start() const noexcept;
    void on_scan_stop() const noexcept;
    void on_scan_updated(int64_t peripheral_id) const noexcept;
    void on_scan_found(int64_t peripheral_id) const noexcept;

    // NOTE: The Linux pairing calls run on the BlueZ thread that waits for the answer, and throw on Java exceptions.
    std::optional<std::string> on_passkey_request(const std::string& peer) const;
    void on_passkey_display(const std::string& peer, const std::string& passkey) const;
    bool on_numeric_comparison(const std::string& peer, const std::string& passkey) const;
    bool on_pairing_authorization(const std::string& peer) const;

  private:
    static jmethodID _method_on_power_on;
    static jmethodID _method_on_power_off;
    static jmethodID _method_on_scan_start;
    static jmethodID _method_on_scan_stop;
    static jmethodID _method_on_scan_updated;
    static jmethodID _method_on_scan_found;
    static jmethodID _method_on_passkey_request;
    static jmethodID _method_on_passkey_display;
    static jmethodID _method_on_numeric_comparison;
    static jmethodID _method_on_pairing_authorization;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<AdapterCallback> registrar;

    SimpleJNI::Object<SimpleJNI::WeakRef> _obj;
};

}  // namespace SimpleKtBLE
}  // namespace Org
