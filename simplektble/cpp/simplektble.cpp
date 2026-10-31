#include <jni.h>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include <fmt/format.h>
#include <simpleble/Advanced.h>
#include <simpleble/Backend.h>
#include <simpleble/Config.h>
#include <simpleble/Logging.h>
#include <simpleble/SimpleBLE.h>
#include <simpleble/Utils.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "NativeCache.h"
#include "org/simplektble/Characteristic.h"
#include "org/simplektble/Descriptor.h"
#include "org/simplektble/Service.h"
#include "org/simplektble/SimpleKtBleException.h"
#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"
#include "simplejni/ThreadRunner.hpp"
#include "simplejni/VM.hpp"
#include "simplejni/java/lang/Integer.hpp"
#include "simplejni/java/util/ArrayList.hpp"
#include "simplejni/java/util/HashMap.hpp"

namespace {

namespace Kt = Org::SimpleKtBLE;
using Kt::NativeCache;
using SimpleJNI::ThreadRunner;

SimpleBLE::ErrorCode error_code(const SimpleBLE::Exception::BaseException& exception) noexcept {
    try {
        return exception.make_error()->code;
    } catch (...) {
        return SimpleBLE::ErrorCode::OUT_OF_MEMORY;
    }
}

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
    } catch (const SimpleBLE::Exception::BaseException& exception) {
        Kt::SimpleKtBleException::throw_new(env, error_code(exception), exception.what());
    } catch (const std::bad_alloc& exception) {
        Kt::SimpleKtBleException::throw_new(env, SimpleBLE::ErrorCode::OUT_OF_MEMORY, exception.what());
    } catch (const std::exception& exception) {
        Kt::SimpleKtBleException::throw_new(env, SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, exception.what());
    } catch (...) {
        Kt::SimpleKtBleException::throw_new(env, SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown native error");
    }

    if constexpr (!std::is_void_v<ReturnType>) return ReturnType{};
}

// NOTE: Never destroyed. At JVM exit its thread cannot detach, so joining it from a static destructor hangs.
ThreadRunner& callback_runner = *new ThreadRunner("SimpleKtBLE");

void dispatch_adapter_event(int64_t adapter_id, void (Kt::AdapterCallback::*event)() const noexcept) {
    callback_runner.enqueue([adapter_id, event] {
        if (auto callback = NativeCache::get().adapter_callback(adapter_id)) {
            (callback.get()->*event)();
        }
    });
}
void dispatch_scan_peripheral(int64_t adapter_id, SimpleBLE::Peripheral peripheral, bool found) noexcept {
    try {
        const int64_t peripheral_id = NativeCache::get().add_peripheral(adapter_id, std::move(peripheral));
        callback_runner.enqueue([adapter_id, peripheral_id, found] {
            if (auto callback = NativeCache::get().adapter_callback(adapter_id)) {
                if (found) {
                    callback->on_scan_found(peripheral_id);
                } else {
                    callback->on_scan_updated(peripheral_id);
                }
            }
        });
    } catch (const std::exception& exception) {
        SimpleBLE::Logging::Logger::get()->log(
            SimpleBLE::Logging::Level::Error, "SimpleKtBLE", __FILE__, __LINE__, __func__,
            std::string("Failed to dispatch native callback: ") + exception.what());
    }
}

void subscribe(int64_t adapter_id, int64_t peripheral_id, const std::string& service, const std::string& characteristic,
               jobject callback, bool indicate) {
    auto data_callback = NativeCache::get().add_data_callback(adapter_id, peripheral_id, service, characteristic,
                                                              callback);
    if (!data_callback) {
        throw SimpleBLE::Exception::OperationFailed(std::string(indicate ? "An indication" : "A notification") +
                                                    " collector is already active for characteristic " +
                                                    characteristic);
    }

    std::weak_ptr<Kt::DataCallback> weak_callback = data_callback;
    auto on_data = [weak_callback](SimpleBLE::ByteArray payload) {
        callback_runner.enqueue([weak_callback, payload = std::move(payload)] {
            if (auto callback = weak_callback.lock()) callback->on_data_received(payload);
        });
    };

    try {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        if (indicate) {
            peripheral.indicate(service, characteristic, std::move(on_data));
        } else {
            peripheral.notify(service, characteristic, std::move(on_data));
        }
    } catch (...) {
        NativeCache::get().remove_data_callback(adapter_id, peripheral_id, service, characteristic, data_callback);
        throw;
    }
}

// NOTE: These are SimpleBLE's advanced functions, which only the platforms' own backends have.
std::vector<SimpleBLE::Peripheral> retrieve_cached_peripherals(
    SimpleBLE::Adapter& adapter, const std::vector<SimpleBLE::BluetoothAddress>& identifiers) {
#if SIMPLEBLE_BACKEND_MACOS
    return SimpleBLE::Advanced::MacOS::retrieve_cached_peripherals(adapter, identifiers);
#else
    (void)adapter;
    (void)identifiers;
    throw SimpleBLE::Exception::OperationNotSupported();
#endif
}

void set_advertisement_local_name(SimpleBLE::Local::Peripheral& peripheral, std::optional<std::string> name) {
#if SIMPLEBLE_BACKEND_MACOS
    SimpleBLE::Advanced::MacOS::set_advertisement_local_name(peripheral, std::move(name));
#elif SIMPLEBLE_BACKEND_LINUX
    SimpleBLE::Advanced::Linux::set_advertisement_local_name(peripheral, std::move(name));
#else
    (void)peripheral;
    (void)name;
    throw SimpleBLE::Exception::OperationNotSupported();
#endif
}

// NOTE: Advanced.h declares the Linux pairing functions on every Linux build, PLAIN included, so SimpleBLE decides
// which adapters take them. Without BlueZ, no adapter does.
#if defined(__linux__) && !defined(__ANDROID__)
using SimpleBLE::Advanced::Linux::set_numeric_comparison_callback;
using SimpleBLE::Advanced::Linux::set_pairing_authorization_callback;
using SimpleBLE::Advanced::Linux::set_passkey_display_callback;
using SimpleBLE::Advanced::Linux::set_passkey_request_callback;
#else
template <typename Callback>
void set_passkey_request_callback(SimpleBLE::Adapter&, const Callback&) {
    throw SimpleBLE::Exception::OperationNotSupported();
}

template <typename Callback>
void set_passkey_display_callback(SimpleBLE::Adapter&, const Callback&) {
    throw SimpleBLE::Exception::OperationNotSupported();
}

template <typename Callback>
void set_numeric_comparison_callback(SimpleBLE::Adapter&, const Callback&) {
    throw SimpleBLE::Exception::OperationNotSupported();
}

template <typename Callback>
void set_pairing_authorization_callback(SimpleBLE::Adapter&, const Callback&) {
    throw SimpleBLE::Exception::OperationNotSupported();
}
#endif

// NOTE: BlueZ keeps its adapters, and their pairing callbacks, after SimpleKtBLE releases them.
void clear_pairing_callbacks(SimpleBLE::Adapter& adapter) {
#if defined(__linux__) && !defined(__ANDROID__)
    try {
        set_passkey_request_callback(adapter, nullptr);
        set_passkey_display_callback(adapter, nullptr);
        set_numeric_comparison_callback(adapter, nullptr);
        set_pairing_authorization_callback(adapter, nullptr);
    } catch (const SimpleBLE::Exception::OperationNotSupported&) {
        // NOTE: Other adapters, such as a Dongl's, have no pairing callbacks.
    }
#else
    (void)adapter;
#endif
}

#if defined(__ANDROID__)
void configure_logging() {
    SimpleBLE::Logging::Logger::get()->set_callback([](SimpleBLE::Logging::Level level, const std::string& module,
                                                       const std::string& file, uint32_t line,
                                                       const std::string& function, const std::string& message) {
        const std::string log_message = fmt::format("{}: {}:{} in {}: {}\n", module, file, line, function, message);

        int android_log_level = ANDROID_LOG_UNKNOWN;
        switch (level) {
            case SimpleBLE::Logging::Level::Verbose:
                android_log_level = ANDROID_LOG_VERBOSE;
                break;
            case SimpleBLE::Logging::Level::Debug:
                android_log_level = ANDROID_LOG_DEBUG;
                break;
            case SimpleBLE::Logging::Level::Info:
                android_log_level = ANDROID_LOG_INFO;
                break;
            case SimpleBLE::Logging::Level::Warn:
                android_log_level = ANDROID_LOG_WARN;
                break;
            case SimpleBLE::Logging::Level::Error:
                android_log_level = ANDROID_LOG_ERROR;
                break;
            case SimpleBLE::Logging::Level::Fatal:
                android_log_level = ANDROID_LOG_FATAL;
                break;
            case SimpleBLE::Logging::Level::None:
                break;
        }

        __android_log_write(android_log_level, "SimpleBLE", log_message.c_str());
    });
}
#endif

}  // namespace

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;

    try {
        SimpleJNI::VM::jvm(vm);
#if defined(__ANDROID__) && !SIMPLEKTBLE_PLAIN
        SimpleBLE::Advanced::Android::set_jvm(vm);
#endif
        SimpleJNI::Registrar::get().preload(env);
#if defined(__ANDROID__)
        configure_logging();
#endif
        return JNI_VERSION_1_6;
    } catch (const std::exception& exception) {
        const std::string message = std::string("Failed to load SimpleKtBLE JNI: ") + exception.what();
#if defined(__ANDROID__)
        __android_log_write(ANDROID_LOG_ERROR, "SimpleBLE", message.c_str());
#else
        SimpleBLE::Logging::Logger::get()->log(SimpleBLE::Logging::Level::Error, "SimpleKtBLE", __FILE__, __LINE__,
                                               __func__, message);
#endif
        if (env->ExceptionCheck()) env->ExceptionClear();
        return JNI_ERR;
    }
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_version(JNIEnv* env, jobject) {
    return call(env, [] {
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(SimpleBLE::get_simpleble_version()).release();
    });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_simplektble_Native_isBluetoothEnabled(JNIEnv* env, jobject) {
    return call(env, [] { return static_cast<jboolean>(SimpleBLE::Adapter::bluetooth_enabled()); });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplektble_Native_getAdapters(JNIEnv* env, jobject) {
    return call(env, [] {
        std::vector<int64_t> adapter_ids;
        for (auto adapter : SimpleBLE::Adapter::get_adapters()) {
            adapter_ids.push_back(NativeCache::get().add_adapter(std::move(adapter)));
        }
        return SimpleJNI::LongArray<SimpleJNI::ReleasableLocalRef>(adapter_ids).release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterRegister(JNIEnv* env, jobject,
                                                                                           jlong adapter_id,
                                                                                           jobject callback) {
    call(env, [adapter_id, callback] {
        NativeCache::get().set_adapter_callback(adapter_id, callback);
        auto adapter = NativeCache::get().adapter(adapter_id);
        adapter.set_callback_on_power_on(
            [adapter_id] { dispatch_adapter_event(adapter_id, &Kt::AdapterCallback::on_power_on); });
        adapter.set_callback_on_power_off(
            [adapter_id] { dispatch_adapter_event(adapter_id, &Kt::AdapterCallback::on_power_off); });
        adapter.set_callback_on_scan_start(
            [adapter_id] { dispatch_adapter_event(adapter_id, &Kt::AdapterCallback::on_scan_start); });
        adapter.set_callback_on_scan_stop(
            [adapter_id] { dispatch_adapter_event(adapter_id, &Kt::AdapterCallback::on_scan_stop); });
        adapter.set_callback_on_scan_found(
            [adapter_id](SimpleBLE::Peripheral peripheral) { dispatch_scan_peripheral(adapter_id, peripheral, true); });
        adapter.set_callback_on_scan_updated([adapter_id](SimpleBLE::Peripheral peripheral) {
            dispatch_scan_peripheral(adapter_id, peripheral, false);
        });
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterRelease(JNIEnv* env, jobject, jlong adapter_id) {
    call(env, [adapter_id] {
        // NOTE: SimpleBLE keeps an adapter's callbacks while anything holds the adapter, so they are cleared first.
        auto adapter = NativeCache::get().adapter(adapter_id);
        adapter.set_callback_on_power_on({});
        adapter.set_callback_on_power_off({});
        adapter.set_callback_on_scan_start({});
        adapter.set_callback_on_scan_stop({});
        adapter.set_callback_on_scan_found({});
        adapter.set_callback_on_scan_updated({});
        clear_pairing_callbacks(adapter);

        // NOTE: The subscriptions end after the values queued before them, as on a disconnection.
        auto ended = NativeCache::get().remove_adapter(adapter_id);
        callback_runner.enqueue([ended = std::move(ended)] {
            for (const auto& data_callback : ended) data_callback->on_ended();
        });
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_adapterIdentifier(JNIEnv* env, jobject,
                                                                                                jlong adapter_id) {
    return call(env, [adapter_id] {
        auto adapter = NativeCache::get().adapter(adapter_id);
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(adapter.identifier()).release();
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_adapterAddress(JNIEnv* env, jobject,
                                                                                             jlong adapter_id) {
    return call(env, [adapter_id] {
        auto adapter = NativeCache::get().adapter(adapter_id);
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(adapter.address()).release();
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_adapterIsPowered(JNIEnv* env, jobject,
                                                                                  jlong adapter_id) {
    return call(
        env, [adapter_id] { return static_cast<jboolean>(NativeCache::get().adapter(adapter_id).is_powered()); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterPowerOn(JNIEnv* env, jobject, jlong adapter_id) {
    call(env, [adapter_id] { NativeCache::get().adapter(adapter_id).power_on(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterPowerOff(JNIEnv* env, jobject, jlong adapter_id) {
    call(env, [adapter_id] { NativeCache::get().adapter(adapter_id).power_off(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterScanStart(JNIEnv* env, jobject,
                                                                                            jlong adapter_id) {
    call(env, [adapter_id] { NativeCache::get().adapter(adapter_id).scan_start(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterScanStop(JNIEnv* env, jobject,
                                                                                           jlong adapter_id) {
    call(env, [adapter_id] { NativeCache::get().adapter(adapter_id).scan_stop(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterScanFor(JNIEnv* env, jobject,
                                                                                          jlong adapter_id,
                                                                                          jint timeout) {
    call(env, [adapter_id, timeout] { NativeCache::get().adapter(adapter_id).scan_for(timeout); });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_adapterScanIsActive(JNIEnv* env, jobject,
                                                                                                   jlong adapter_id) {
    return call(
        env, [adapter_id] { return static_cast<jboolean>(NativeCache::get().adapter(adapter_id).scan_is_active()); });
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_org_simplektble_Native_adapterScanGetResults(JNIEnv* env, jobject, jlong adapter_id) {
    return call(env, [adapter_id] {
        std::vector<int64_t> peripheral_ids;
        auto adapter = NativeCache::get().adapter(adapter_id);
        for (auto peripheral : adapter.scan_get_results()) {
            peripheral_ids.push_back(NativeCache::get().add_peripheral(adapter_id, std::move(peripheral)));
        }
        return SimpleJNI::LongArray<SimpleJNI::ReleasableLocalRef>(peripheral_ids).release();
    });
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_org_simplektble_Native_adapterGetPairedPeripherals(JNIEnv* env, jobject, jlong adapter_id) {
    return call(env, [adapter_id] {
        std::vector<int64_t> peripheral_ids;
        auto adapter = NativeCache::get().adapter(adapter_id);
        for (auto peripheral : adapter.get_paired_peripherals()) {
            peripheral_ids.push_back(NativeCache::get().add_peripheral(adapter_id, std::move(peripheral)));
        }
        return SimpleJNI::LongArray<SimpleJNI::ReleasableLocalRef>(peripheral_ids).release();
    });
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_org_simplektble_Native_adapterGetConnectedPeripherals(JNIEnv* env, jobject, jlong adapter_id) {
    return call(env, [adapter_id] {
        std::vector<int64_t> peripheral_ids;
        auto adapter = NativeCache::get().adapter(adapter_id);
        for (auto peripheral : adapter.get_connected_peripherals()) {
            peripheral_ids.push_back(NativeCache::get().add_peripheral(adapter_id, std::move(peripheral)));
        }
        return SimpleJNI::LongArray<SimpleJNI::ReleasableLocalRef>(peripheral_ids).release();
    });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplektble_Native_adapterRetrieveCachedPeripherals(
    JNIEnv* env, jobject, jlong adapter_id, jobjectArray j_identifiers) {
    return call(env, [env, adapter_id, j_identifiers] {
        std::vector<SimpleBLE::BluetoothAddress> identifiers;
        const jsize count = env->GetArrayLength(j_identifiers);
        for (jsize index = 0; index < count; ++index) {
            auto value = static_cast<jstring>(env->GetObjectArrayElement(j_identifiers, index));
            SimpleJNI::Exception::check(env);
            identifiers.push_back(SimpleJNI::String<SimpleJNI::LocalRef>(value).str());
            env->DeleteLocalRef(value);
        }

        auto adapter = NativeCache::get().adapter(adapter_id);
        std::vector<int64_t> peripheral_ids;
        for (auto peripheral : retrieve_cached_peripherals(adapter, identifiers)) {
            peripheral_ids.push_back(NativeCache::get().add_peripheral(adapter_id, std::move(peripheral)));
        }
        return SimpleJNI::LongArray<SimpleJNI::ReleasableLocalRef>(peripheral_ids).release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterSetPasskeyRequestHandler(
    JNIEnv* env, jobject, jlong adapter_id, jboolean enabled) {
    call(env, [adapter_id, enabled] {
        auto adapter = NativeCache::get().adapter(adapter_id);
        std::function<std::optional<std::string>(SimpleBLE::BluetoothAddress)> on_request;
        if (enabled) {
            on_request = [adapter_id](SimpleBLE::BluetoothAddress peer) -> std::optional<std::string> {
                auto callback = NativeCache::get().adapter_callback(adapter_id);
                if (!callback) return std::nullopt;
                return callback->on_passkey_request(peer);
            };
        }
        set_passkey_request_callback(adapter, on_request);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterSetPasskeyDisplayHandler(
    JNIEnv* env, jobject, jlong adapter_id, jboolean enabled) {
    call(env, [adapter_id, enabled] {
        auto adapter = NativeCache::get().adapter(adapter_id);
        std::function<void(SimpleBLE::BluetoothAddress, const std::string&)> on_display;
        if (enabled) {
            on_display = [adapter_id](SimpleBLE::BluetoothAddress peer, const std::string& passkey) {
                if (auto callback = NativeCache::get().adapter_callback(adapter_id)) {
                    callback->on_passkey_display(peer, passkey);
                }
            };
        }
        set_passkey_display_callback(adapter, on_display);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterSetNumericComparisonHandler(
    JNIEnv* env, jobject, jlong adapter_id, jboolean enabled) {
    call(env, [adapter_id, enabled] {
        auto adapter = NativeCache::get().adapter(adapter_id);
        std::function<bool(SimpleBLE::BluetoothAddress, const std::string&)> on_compare;
        if (enabled) {
            on_compare = [adapter_id](SimpleBLE::BluetoothAddress peer, const std::string& passkey) {
                auto callback = NativeCache::get().adapter_callback(adapter_id);
                return callback && callback->on_numeric_comparison(peer, passkey);
            };
        }
        set_numeric_comparison_callback(adapter, on_compare);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_adapterSetPairingAuthorizationHandler(
    JNIEnv* env, jobject, jlong adapter_id, jboolean enabled) {
    call(env, [adapter_id, enabled] {
        auto adapter = NativeCache::get().adapter(adapter_id);
        std::function<bool(SimpleBLE::BluetoothAddress)> on_authorize;
        if (enabled) {
            on_authorize = [adapter_id](SimpleBLE::BluetoothAddress peer) {
                // NOTE: Without a handler, Just Works pairing is accepted, as SimpleBLE does without a callback.
                auto callback = NativeCache::get().adapter_callback(adapter_id);
                return !callback || callback->on_pairing_authorization(peer);
            };
        }
        set_pairing_authorization_callback(adapter, on_authorize);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralRegister(JNIEnv* env, jobject,
                                                                                                 jlong adapter_id,
                                                                                                 jlong peripheral_id,
                                                                                                 jobject callback) {
    call(env, [adapter_id, peripheral_id, callback] {
        NativeCache::get().set_peripheral_callback(adapter_id, peripheral_id, callback);
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        peripheral.set_callback_on_connected([adapter_id, peripheral_id] {
            callback_runner.enqueue([adapter_id, peripheral_id] {
                if (auto callback = NativeCache::get().peripheral_callback(adapter_id, peripheral_id)) {
                    callback->on_connected();
                }
            });
        });
        peripheral.set_callback_on_disconnected([adapter_id, peripheral_id] {
            // NOTE: The disconnection ends the subscriptions it had, and only those, after the values queued before it.
            auto ended = NativeCache::get().take_data_callbacks(adapter_id, peripheral_id);
            callback_runner.enqueue([adapter_id, peripheral_id, ended = std::move(ended)] {
                for (const auto& data_callback : ended) data_callback->on_ended();
                if (auto callback = NativeCache::get().peripheral_callback(adapter_id, peripheral_id)) {
                    callback->on_disconnected();
                }
            });
        });
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_peripheralIdentifier(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(peripheral.identifier()).release();
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_peripheralAddress(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(peripheral.address()).release();
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplektble_Native_peripheralAddressType(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        return static_cast<jint>(NativeCache::get().peripheral(adapter_id, peripheral_id).address_type());
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplektble_Native_peripheralRssi(JNIEnv* env, jobject,
                                                                                             jlong adapter_id,
                                                                                             jlong peripheral_id) {
    return call(
        env, [adapter_id, peripheral_id] { return NativeCache::get().peripheral(adapter_id, peripheral_id).rssi(); });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplektble_Native_peripheralTxPower(JNIEnv* env, jobject,
                                                                                                jlong adapter_id,
                                                                                                jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        return NativeCache::get().peripheral(adapter_id, peripheral_id).tx_power();
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplektble_Native_peripheralMtu(JNIEnv* env, jobject,
                                                                                            jlong adapter_id,
                                                                                            jlong peripheral_id) {
    return call(
        env, [adapter_id, peripheral_id] { return NativeCache::get().peripheral(adapter_id, peripheral_id).mtu(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralConnect(JNIEnv* env, jobject,
                                                                                                jlong adapter_id,
                                                                                                jlong peripheral_id) {
    call(
        env, [adapter_id, peripheral_id] { NativeCache::get().peripheral(adapter_id, peripheral_id).connect(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralDisconnect(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    call(
        env, [adapter_id, peripheral_id] { NativeCache::get().peripheral(adapter_id, peripheral_id).disconnect(); });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_peripheralIsConnected(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        return static_cast<jboolean>(NativeCache::get().peripheral(adapter_id, peripheral_id).is_connected());
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_peripheralIsConnectable(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        return static_cast<jboolean>(NativeCache::get().peripheral(adapter_id, peripheral_id).is_connectable());
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_peripheralIsPaired(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        return static_cast<jboolean>(NativeCache::get().peripheral(adapter_id, peripheral_id).is_paired());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralUnpair(JNIEnv* env, jobject,
                                                                                               jlong adapter_id,
                                                                                               jlong peripheral_id) {
    call(
        env, [adapter_id, peripheral_id] { NativeCache::get().peripheral(adapter_id, peripheral_id).unpair(); });
}

extern "C" JNIEXPORT jobject JNICALL Java_org_simplektble_Native_peripheralServices(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        SimpleJNI::Java::Util::ArrayList services;

        for (auto service : peripheral.services()) {
            SimpleJNI::Java::Util::ArrayList characteristics;

            for (auto characteristic : service.characteristics()) {
                SimpleJNI::Java::Util::ArrayList descriptors;

                for (auto descriptor : characteristic.descriptors()) {
                    Kt::Descriptor descriptor_object(descriptor.uuid());
                    descriptors.add(descriptor_object);
                }

                Kt::Characteristic characteristic_object(
                    characteristic.uuid(), descriptors, characteristic.can_read(),
                    characteristic.can_write_request(), characteristic.can_write_command(),
                    characteristic.can_notify(), characteristic.can_indicate());
                characteristics.add(characteristic_object);
            }

            Kt::Service service_object(service.uuid(), SimpleJNI::ByteArray<SimpleJNI::LocalRef>(service.data()),
                                       characteristics);
            services.add(service_object);
        }

        return services.release();
    });
}

extern "C" JNIEXPORT jobject JNICALL Java_org_simplektble_Native_peripheralManufacturerData(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id) {
    return call(env, [adapter_id, peripheral_id] {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        SimpleJNI::Java::Util::HashMap manufacturer_data;

        for (const auto& [company_id, data] : peripheral.manufacturer_data()) {
            SimpleJNI::Java::Lang::Integer key(static_cast<jint>(company_id));
            SimpleJNI::ByteArray<SimpleJNI::LocalRef> value(data);
            manufacturer_data.put(key.get(), value.get());
        }

        return manufacturer_data.release();
    });
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_simplektble_Native_peripheralRead(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic) {
    return call(env, [adapter_id, peripheral_id, j_service, j_characteristic] {
        SimpleJNI::String<SimpleJNI::LocalRef> service(j_service);
        SimpleJNI::String<SimpleJNI::LocalRef> characteristic(j_characteristic);
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        return SimpleJNI::ByteArray<SimpleJNI::ReleasableLocalRef>(peripheral.read(service.str(), characteristic.str()))
            .release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralWriteRequest(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jbyteArray j_data) {
    call(env, [adapter_id, peripheral_id, j_service, j_characteristic, j_data] {
        SimpleJNI::String<SimpleJNI::LocalRef> service(j_service);
        SimpleJNI::String<SimpleJNI::LocalRef> characteristic(j_characteristic);
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> data(j_data);
        NativeCache::get()
            .peripheral(adapter_id, peripheral_id)
            .write_request(service.str(), characteristic.str(), data.bytes());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralWriteCommand(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jbyteArray j_data) {
    call(env, [adapter_id, peripheral_id, j_service, j_characteristic, j_data] {
        SimpleJNI::String<SimpleJNI::LocalRef> service(j_service);
        SimpleJNI::String<SimpleJNI::LocalRef> characteristic(j_characteristic);
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> data(j_data);
        NativeCache::get()
            .peripheral(adapter_id, peripheral_id)
            .write_command(service.str(), characteristic.str(), data.bytes());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralNotify(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jobject callback) {
    call(env, [adapter_id, peripheral_id, j_service, j_characteristic, callback] {
        SimpleJNI::String<SimpleJNI::LocalRef> service(j_service);
        SimpleJNI::String<SimpleJNI::LocalRef> characteristic(j_characteristic);
        subscribe(adapter_id, peripheral_id, service.str(), characteristic.str(), callback, false);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralIndicate(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jobject callback) {
    call(env, [adapter_id, peripheral_id, j_service, j_characteristic, callback] {
        SimpleJNI::String<SimpleJNI::LocalRef> service(j_service);
        SimpleJNI::String<SimpleJNI::LocalRef> characteristic(j_characteristic);
        subscribe(adapter_id, peripheral_id, service.str(), characteristic.str(), callback, true);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralUnsubscribe(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic) {
    call(env, [adapter_id, peripheral_id, j_service, j_characteristic] {
        SimpleJNI::String<SimpleJNI::LocalRef> service_value(j_service);
        SimpleJNI::String<SimpleJNI::LocalRef> characteristic_value(j_characteristic);
        const std::string service = service_value.str();
        const std::string characteristic = characteristic_value.str();
        try {
            NativeCache::get().peripheral(adapter_id, peripheral_id).unsubscribe(service, characteristic);
        } catch (...) {
            NativeCache::get().remove_data_callback(adapter_id, peripheral_id, service, characteristic);
            throw;
        }
        NativeCache::get().remove_data_callback(adapter_id, peripheral_id, service, characteristic);
    });
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_simplektble_Native_peripheralDescriptorRead(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jstring j_descriptor) {
    return call(env, [adapter_id, peripheral_id, j_service, j_characteristic, j_descriptor] {
        SimpleJNI::String<SimpleJNI::LocalRef> service(j_service);
        SimpleJNI::String<SimpleJNI::LocalRef> characteristic(j_characteristic);
        SimpleJNI::String<SimpleJNI::LocalRef> descriptor(j_descriptor);
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        return SimpleJNI::ByteArray<SimpleJNI::ReleasableLocalRef>(
                   peripheral.read(service.str(), characteristic.str(), descriptor.str()))
            .release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralDescriptorWrite(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jstring j_descriptor, jbyteArray j_data) {
    call(env, [adapter_id, peripheral_id, j_service, j_characteristic, j_descriptor, j_data] {
        SimpleJNI::String<SimpleJNI::LocalRef> service(j_service);
        SimpleJNI::String<SimpleJNI::LocalRef> characteristic(j_characteristic);
        SimpleJNI::String<SimpleJNI::LocalRef> descriptor(j_descriptor);
        SimpleJNI::ByteArray<SimpleJNI::LocalRef> data(j_data);
        NativeCache::get()
            .peripheral(adapter_id, peripheral_id)
            .write(service.str(), characteristic.str(), descriptor.str(), data.bytes());
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplektble_Native_adapterCreateLocalPeripheralWithContext(
    JNIEnv* env, jobject, jlong adapter_id, jobject application_context) {
    return call(env, [adapter_id, application_context] {
#if defined(__ANDROID__) && !SIMPLEKTBLE_PLAIN
        SimpleBLE::Advanced::Android::set_context(application_context);
#else
        (void)application_context;
#endif
        auto peripheral = NativeCache::get().adapter(adapter_id).create_local_peripheral();
        return static_cast<jlong>(NativeCache::get().add_local_peripheral(std::move(peripheral)));
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralRelease(
    JNIEnv* env, jobject, jlong peripheral_id) {
    call(env, [peripheral_id] { NativeCache::get().remove_local_peripheral(peripheral_id); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localServiceRelease(
    JNIEnv* env, jobject, jlong service_id) {
    call(env, [service_id] { NativeCache::get().remove_local_service(service_id); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localCharacteristicRelease(
    JNIEnv* env, jobject, jlong characteristic_id) {
    call(env, [characteristic_id] { NativeCache::get().remove_local_characteristic(characteristic_id); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralRegister(
    JNIEnv* env, jobject, jlong peripheral_id, jobject callback) {
    call(env, [peripheral_id, callback] {
        NativeCache::get().set_local_peripheral_callback(peripheral_id, callback);
        auto peripheral = NativeCache::get().local_peripheral(peripheral_id);
        peripheral.set_callback_on_client_connected([peripheral_id](SimpleBLE::BluetoothAddress address) {
            callback_runner.enqueue([peripheral_id, address = std::move(address)] {
                if (auto callback = NativeCache::get().local_peripheral_callback(peripheral_id)) {
                    callback->on_client_connected(address);
                }
            });
        });
        peripheral.set_callback_on_client_disconnected([peripheral_id](SimpleBLE::BluetoothAddress address) {
            callback_runner.enqueue([peripheral_id, address = std::move(address)] {
                if (auto callback = NativeCache::get().local_peripheral_callback(peripheral_id)) {
                    callback->on_client_disconnected(address);
                }
            });
        });
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralAddAdvertisedService(
    JNIEnv* env, jobject, jlong peripheral_id, jobjectArray service_uuids) {
    call(env, [env, peripheral_id, service_uuids] {
        std::vector<SimpleBLE::BluetoothUUID> advertised_service_uuids;
        if (service_uuids != nullptr) {
            const jsize count = env->GetArrayLength(service_uuids);
            advertised_service_uuids.reserve(count);
            for (jsize index = 0; index < count; ++index) {
                auto value = static_cast<jstring>(env->GetObjectArrayElement(service_uuids, index));
                SimpleJNI::Exception::check(env);
                advertised_service_uuids.push_back(SimpleJNI::String<SimpleJNI::LocalRef>(value).str());
                env->DeleteLocalRef(value);
            }
        }
        NativeCache::get().local_peripheral(peripheral_id).add_advertised_service(std::move(advertised_service_uuids));
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralSetAdvertisedManufacturerData(
    JNIEnv* env, jobject, jlong peripheral_id, jint company_id, jbyteArray data) {
    call(env, [peripheral_id, company_id, data] {
        NativeCache::get().local_peripheral(peripheral_id).set_advertised_manufacturer_data(
            static_cast<uint16_t>(company_id), SimpleJNI::ByteArray<SimpleJNI::LocalRef>(data).bytes());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralSetConnectable(
    JNIEnv* env, jobject, jlong peripheral_id, jboolean connectable) {
    call(env, [peripheral_id, connectable] {
        NativeCache::get().local_peripheral(peripheral_id).set_connectable(connectable);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralSetAdvertisementLocalName(
    JNIEnv* env, jobject, jlong peripheral_id, jstring j_name) {
    call(env, [peripheral_id, j_name] {
        std::optional<std::string> name;
        if (j_name != nullptr) name = SimpleJNI::String<SimpleJNI::LocalRef>(j_name).str();

        auto peripheral = NativeCache::get().local_peripheral(peripheral_id);
        set_advertisement_local_name(peripheral, std::move(name));
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplektble_Native_localPeripheralAddService(
    JNIEnv* env, jobject, jlong peripheral_id, jstring uuid) {
    return call(env, [peripheral_id, uuid] {
        auto service = NativeCache::get().local_peripheral(peripheral_id).add_service(
            SimpleJNI::String<SimpleJNI::LocalRef>(uuid).str());
        return static_cast<jlong>(NativeCache::get().add_local_service(std::move(service)));
    });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplektble_Native_localPeripheralServices(
    JNIEnv* env, jobject, jlong peripheral_id) {
    return call(env, [peripheral_id] {
        std::vector<int64_t> service_ids;
        for (auto service : NativeCache::get().local_peripheral(peripheral_id).services()) {
            service_ids.push_back(NativeCache::get().add_local_service(std::move(service)));
        }
        return SimpleJNI::LongArray<SimpleJNI::ReleasableLocalRef>(service_ids).release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralRemoveAllServices(
    JNIEnv* env, jobject, jlong peripheral_id) {
    call(env, [peripheral_id] { NativeCache::get().local_peripheral(peripheral_id).remove_all_services(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralStart(
    JNIEnv* env, jobject, jlong peripheral_id) {
    call(env, [peripheral_id] { NativeCache::get().local_peripheral(peripheral_id).start(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localPeripheralStop(
    JNIEnv* env, jobject, jlong peripheral_id) {
    call(env, [peripheral_id] { NativeCache::get().local_peripheral(peripheral_id).stop(); });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_localPeripheralIsStarted(
    JNIEnv* env, jobject, jlong peripheral_id) {
    return call(env, [peripheral_id] {
        return static_cast<jboolean>(NativeCache::get().local_peripheral(peripheral_id).is_started());
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_localPeripheralIsAdvertising(
    JNIEnv* env, jobject, jlong peripheral_id) {
    return call(env, [peripheral_id] {
        return static_cast<jboolean>(NativeCache::get().local_peripheral(peripheral_id).is_advertising());
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_localServiceUuid(
    JNIEnv* env, jobject, jlong service_id) {
    return call(env, [service_id] {
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(
                   NativeCache::get().local_service(service_id).uuid())
            .release();
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplektble_Native_localServiceAddCharacteristic(
    JNIEnv* env, jobject, jlong service_id, jstring uuid, jint capabilities) {
    return call(env, [service_id, uuid, capabilities] {
        using Capability = SimpleBLE::Local::CharacteristicCapability;
        std::set<Capability> native_capabilities;
        if ((capabilities & 1) != 0) native_capabilities.insert(Capability::READ);
        if ((capabilities & 2) != 0) native_capabilities.insert(Capability::WRITE_REQUEST);
        if ((capabilities & 4) != 0) native_capabilities.insert(Capability::WRITE_COMMAND);
        if ((capabilities & 8) != 0) native_capabilities.insert(Capability::NOTIFY);
        if ((capabilities & 16) != 0) native_capabilities.insert(Capability::INDICATE);
        auto characteristic = NativeCache::get().local_service(service_id).add_characteristic(
            SimpleJNI::String<SimpleJNI::LocalRef>(uuid).str(), std::move(native_capabilities));
        return static_cast<jlong>(NativeCache::get().add_local_characteristic(std::move(characteristic)));
    });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplektble_Native_localServiceCharacteristics(
    JNIEnv* env, jobject, jlong service_id) {
    return call(env, [service_id] {
        std::vector<int64_t> characteristic_ids;
        for (auto characteristic : NativeCache::get().local_service(service_id).characteristics()) {
            characteristic_ids.push_back(NativeCache::get().add_local_characteristic(std::move(characteristic)));
        }
        return SimpleJNI::LongArray<SimpleJNI::ReleasableLocalRef>(characteristic_ids).release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localCharacteristicRegister(
    JNIEnv* env, jobject, jlong characteristic_id, jobject callback) {
    call(env, [characteristic_id, callback] {
        NativeCache::get().set_local_characteristic_callback(characteristic_id, callback);
        auto characteristic = NativeCache::get().local_characteristic(characteristic_id);
        characteristic.set_callback_on_write([characteristic_id](SimpleBLE::ByteArray value) {
            callback_runner.enqueue([characteristic_id, value = std::move(value)] {
                if (auto callback = NativeCache::get().local_characteristic_callback(characteristic_id)) {
                    callback->on_write(value);
                }
            });
        });
        characteristic.set_callback_on_subscribed([characteristic_id] {
            callback_runner.enqueue([characteristic_id] {
                if (auto callback = NativeCache::get().local_characteristic_callback(characteristic_id)) {
                    callback->on_subscribed();
                }
            });
        });
        characteristic.set_callback_on_unsubscribed([characteristic_id] {
            callback_runner.enqueue([characteristic_id] {
                if (auto callback = NativeCache::get().local_characteristic_callback(characteristic_id)) {
                    callback->on_unsubscribed();
                }
            });
        });
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_localCharacteristicUuid(
    JNIEnv* env, jobject, jlong characteristic_id) {
    return call(env, [characteristic_id] {
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(
                   NativeCache::get().local_characteristic(characteristic_id).uuid())
            .release();
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplektble_Native_localCharacteristicCapabilities(
    JNIEnv* env, jobject, jlong characteristic_id) {
    return call(env, [characteristic_id] {
        using Capability = SimpleBLE::Local::CharacteristicCapability;
        const auto capabilities = NativeCache::get().local_characteristic(characteristic_id).capabilities();
        jint mask = 0;
        if (capabilities.count(Capability::READ) > 0) mask |= 1;
        if (capabilities.count(Capability::WRITE_REQUEST) > 0) mask |= 2;
        if (capabilities.count(Capability::WRITE_COMMAND) > 0) mask |= 4;
        if (capabilities.count(Capability::NOTIFY) > 0) mask |= 8;
        if (capabilities.count(Capability::INDICATE) > 0) mask |= 16;
        return mask;
    });
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_simplektble_Native_localCharacteristicValue(
    JNIEnv* env, jobject, jlong characteristic_id) {
    return call(env, [characteristic_id] {
        return SimpleJNI::ByteArray<SimpleJNI::ReleasableLocalRef>(
                   NativeCache::get().local_characteristic(characteristic_id).value())
            .release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localCharacteristicSetValue(
    JNIEnv* env, jobject, jlong characteristic_id, jbyteArray value) {
    call(env, [characteristic_id, value] {
        NativeCache::get().local_characteristic(characteristic_id).set_value(
            SimpleJNI::ByteArray<SimpleJNI::LocalRef>(value).bytes());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_localCharacteristicSetReadHandler(
    JNIEnv* env, jobject, jlong characteristic_id, jboolean enabled) {
    call(env, [characteristic_id, enabled] {
        auto characteristic = NativeCache::get().local_characteristic(characteristic_id);
        if (enabled) {
            characteristic.set_callback_on_read([characteristic_id] {
                // If the handler fails or returns null, serve the stored value instead.
                if (auto callback = NativeCache::get().local_characteristic_callback(characteristic_id)) {
                    if (auto value = callback->on_read()) return *value;
                }
                return NativeCache::get().local_characteristic(characteristic_id).value();
            });
        } else {
            characteristic.set_callback_on_read({});
        }
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralSetPasskeyRequestHandler(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jboolean enabled) {
    call(env, [adapter_id, peripheral_id, enabled] {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        if (!enabled) {
            SimpleBLE::Advanced::Dongl::set_passkey_request_callback(peripheral, nullptr);
            return;
        }
        SimpleBLE::Advanced::Dongl::set_passkey_request_callback(
            peripheral, [adapter_id, peripheral_id]() -> std::optional<std::string> {
                auto callback = NativeCache::get().peripheral_callback(adapter_id, peripheral_id);
                if (!callback) return std::nullopt;
                return callback->on_passkey_request();
            });
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralSetPasskeyDisplayHandler(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jboolean enabled) {
    call(env, [adapter_id, peripheral_id, enabled] {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        if (!enabled) {
            SimpleBLE::Advanced::Dongl::set_passkey_display_callback(peripheral, nullptr);
            return;
        }
        SimpleBLE::Advanced::Dongl::set_passkey_display_callback(
            peripheral, [adapter_id, peripheral_id](const std::string& passkey) {
                if (auto callback = NativeCache::get().peripheral_callback(adapter_id, peripheral_id)) {
                    callback->on_passkey_display(passkey);
                }
            });
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralSetNumericComparisonHandler(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jboolean enabled) {
    call(env, [adapter_id, peripheral_id, enabled] {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        if (!enabled) {
            SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(peripheral, nullptr);
            return;
        }
        SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(
            peripheral, [adapter_id, peripheral_id](const std::string& passkey) {
                auto callback = NativeCache::get().peripheral_callback(adapter_id, peripheral_id);
                return callback && callback->on_numeric_comparison(passkey);
            });
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_peripheralSecureConnection(
    JNIEnv* env, jobject, jlong adapter_id, jlong peripheral_id, jboolean mitm) {
    call(env, [adapter_id, peripheral_id, mitm] {
        auto peripheral = NativeCache::get().peripheral(adapter_id, peripheral_id);
        SimpleBLE::Advanced::Dongl::secure_connection(peripheral, mitm);
    });
}

extern "C" JNIEXPORT jobject JNICALL Java_org_simplektble_Native_getBackends(JNIEnv* env, jobject) {
    return call(env, [] {
        SimpleJNI::Java::Util::ArrayList identifiers;
        for (auto backend : SimpleBLE::Backend::get_backends()) {
            SimpleJNI::String<SimpleJNI::LocalRef> identifier(NativeCache::get().add_backend(std::move(backend)));
            identifiers.add(identifier.get());
        }
        return identifiers.release();
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_getNativeBackend(JNIEnv* env, jobject) {
    return call(env, [] {
        auto identifier = NativeCache::get().add_backend(SimpleBLE::Backend::get_native());
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(identifier).release();
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_getDonglBackend(JNIEnv* env, jobject) {
    return call(env, [] {
        auto identifier = NativeCache::get().add_backend(SimpleBLE::Backend::get_dongl());
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(identifier).release();
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplektble_Native_getSimulatorBackend(JNIEnv* env, jobject) {
    return call(env, [] {
        auto identifier = NativeCache::get().add_backend(SimpleBLE::Backend::get_simulator());
        return SimpleJNI::String<SimpleJNI::ReleasableLocalRef>(identifier).release();
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_backendIsBluetoothEnabled(JNIEnv* env, jobject,
                                                                                          jstring j_backend) {
    return call(env, [j_backend] {
        SimpleJNI::String<SimpleJNI::LocalRef> identifier(j_backend);
        return static_cast<jboolean>(NativeCache::get().backend(identifier.str()).bluetooth_enabled());
    });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplektble_Native_backendGetAdapters(JNIEnv* env, jobject,
                                                                                      jstring j_backend) {
    return call(env, [j_backend] {
        SimpleJNI::String<SimpleJNI::LocalRef> identifier(j_backend);
        std::vector<int64_t> adapter_ids;
        for (auto adapter : NativeCache::get().backend(identifier.str()).adapters()) {
            adapter_ids.push_back(NativeCache::get().add_adapter(std::move(adapter)));
        }
        return SimpleJNI::LongArray<SimpleJNI::ReleasableLocalRef>(adapter_ids).release();
    });
}

namespace {

// The keys and groups match ConfigKey and ConfigGroup in Config.kt.
bool& config_boolean(jint key) {
    switch (key) {
        case 0:
            return SimpleBLE::Config::SimpleBluez::use_system_bus;
        case 3:
            return SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread;
        case 4:
            return SimpleBLE::Config::WinRT::use_deferred_disconnect;
        case 5:
            return SimpleBLE::Config::Dongl::use_dongl_backend;
        case 6:
            return SimpleBLE::Config::Dongl::auto_update;
        case 7:
            return SimpleBLE::Config::Dongl::force_update;
        case 9:
            return SimpleBLE::Config::Android::negotiate_mtu;
        case 10:
            return SimpleBLE::Config::SimpleBluez::interactive_pairing;
    }
    throw std::invalid_argument("Unknown boolean setting");
}

SimpleBLE::Config::Android::ConnectionPriorityRequest& config_connection_priority(jint key) {
    if (key == 8) return SimpleBLE::Config::Android::connection_priority_request;
    throw std::invalid_argument("Unknown integer setting");
}

std::chrono::steady_clock::duration& config_duration(jint key) {
    switch (key) {
        case 1:
            return SimpleBLE::Config::SimpleBluez::connection_timeout;
        case 2:
            return SimpleBLE::Config::SimpleBluez::disconnection_timeout;
    }
    throw std::invalid_argument("Unknown duration setting");
}

}  // namespace

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_configReset(JNIEnv* env, jobject, jint group) {
    call(env, [group] {
        switch (group) {
            case 0:
                return SimpleBLE::Config::Base::reset_all();
            case 1:
                return SimpleBLE::Config::SimpleBluez::reset();
            case 2:
                return SimpleBLE::Config::WinRT::reset();
            case 3:
                return SimpleBLE::Config::CoreBluetooth::reset();
            case 4:
                return SimpleBLE::Config::Dongl::reset();
            case 5:
                return SimpleBLE::Config::Android::reset();
        }
        throw std::invalid_argument("Unknown settings group");
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplektble_Native_configGetBoolean(JNIEnv* env, jobject, jint key) {
    return call(env, [key] { return static_cast<jboolean>(config_boolean(key)); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_configSetBoolean(JNIEnv* env, jobject, jint key,
                                                                              jboolean value) {
    call(env, [key, value] { config_boolean(key) = value; });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplektble_Native_configGetInt(JNIEnv* env, jobject, jint key) {
    return call(env, [key] { return static_cast<jint>(config_connection_priority(key)); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_configSetInt(JNIEnv* env, jobject, jint key,
                                                                          jint value) {
    call(env, [key, value] {
        config_connection_priority(key) = static_cast<SimpleBLE::Config::Android::ConnectionPriorityRequest>(value);
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplektble_Native_configGetMilliseconds(JNIEnv* env, jobject, jint key) {
    return call(env, [key] {
        return static_cast<jlong>(std::chrono::duration_cast<std::chrono::milliseconds>(config_duration(key)).count());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplektble_Native_configSetMilliseconds(JNIEnv* env, jobject, jint key,
                                                                                   jlong value) {
    call(env, [key, value] { config_duration(key) = std::chrono::milliseconds(value); });
}
