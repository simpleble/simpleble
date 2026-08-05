#pragma once

#include <cstdint>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleBLE {
namespace Android {

class AdapterCallback {
  public:
    AdapterCallback() = default;
    explicit AdapterCallback(jobject callback);

    void on_scan_start() const noexcept;
    void on_scan_stop() const noexcept;
    void on_scan_updated(int64_t peripheral_id) const noexcept;
    void on_scan_found(int64_t peripheral_id) const noexcept;

  private:
    static jmethodID _method_on_scan_start;
    static jmethodID _method_on_scan_stop;
    static jmethodID _method_on_scan_updated;
    static jmethodID _method_on_scan_found;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<AdapterCallback> registrar;

    SimpleJNI::Object<SimpleJNI::WeakRef> _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
