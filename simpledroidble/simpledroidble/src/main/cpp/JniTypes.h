#pragma once

#include <jni.h>

#include <simpleble/Service.h>
#include <simpleble/Types.h>

#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleDroidJNI {

class AdapterCallback {
  public:
    AdapterCallback() = default;
    explicit AdapterCallback(jobject callback);

    void on_scan_start() const noexcept;
    void on_scan_stop() const noexcept;
    void on_scan_updated(int64_t peripheral_id) const noexcept;
    void on_scan_found(int64_t peripheral_id) const noexcept;

  private:
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _on_scan_start;
    static jmethodID _on_scan_stop;
    static jmethodID _on_scan_updated;
    static jmethodID _on_scan_found;
    static const SimpleJNI::JNIDescriptor _descriptor;
    static const SimpleJNI::AutoRegister<AdapterCallback> _registrar;

    SimpleJNI::Object<SimpleJNI::WeakRef> _callback;
};

class PeripheralCallback {
  public:
    PeripheralCallback() = default;
    explicit PeripheralCallback(jobject callback);

    void on_connected() const noexcept;
    void on_disconnected() const noexcept;

  private:
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _on_connected;
    static jmethodID _on_disconnected;
    static const SimpleJNI::JNIDescriptor _descriptor;
    static const SimpleJNI::AutoRegister<PeripheralCallback> _registrar;

    SimpleJNI::Object<SimpleJNI::WeakRef> _callback;
};

class DataCallback {
  public:
    explicit DataCallback(jobject callback);

    void on_data_received(const SimpleBLE::ByteArray& data) const noexcept;

  private:
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _on_data_received;
    static const SimpleJNI::JNIDescriptor _descriptor;
    static const SimpleJNI::AutoRegister<DataCallback> _registrar;

    SimpleJNI::Object<SimpleJNI::GlobalRef> _callback;
};

jobject to_services(const std::vector<SimpleBLE::Service>& services);
jobject to_manufacturer_data(const std::map<uint16_t, SimpleBLE::ByteArray>& manufacturer_data);

void throw_exception(JNIEnv* env, const std::string& message) noexcept;

template <typename Func>
auto call(JNIEnv* env, Func&& func) noexcept {
    using ReturnType = decltype(func());

    try {
        if constexpr (std::is_void_v<ReturnType>) {
            std::forward<Func>(func)();
            return;
        } else {
            return std::forward<Func>(func)();
        }
    } catch (const std::exception& exception) {
        throw_exception(env, exception.what());
    } catch (...) {
        throw_exception(env, "Unknown native error");
    }

    if constexpr (!std::is_void_v<ReturnType>) {
        return ReturnType{};
    }
}

}  // namespace SimpleDroidJNI
