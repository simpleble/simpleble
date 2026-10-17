#include <jni.h>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include <fmt/format.h>
#include <simpleble/Advanced.h>
#include <simpleble/Logging.h>
#include <simpleble/SimpleBLE.h>
#include <simpleble/Utils.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "NativeCache.h"
#include "ThreadRunner.h"
#include "org/simplektble/Characteristic.h"
#include "org/simplektble/Descriptor.h"
#include "org/simplektble/Service.h"
#include "org/simplektble/SimpleKtBleException.h"
#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"
#include "simplejni/VM.hpp"
#include "simplejni/java/lang/Integer.hpp"
#include "simplejni/java/util/ArrayList.hpp"
#include "simplejni/java/util/HashMap.hpp"

namespace {

namespace Kt = Org::SimpleKtBLE;
using Kt::NativeCache;
using Kt::ThreadRunner;

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
        Kt::SimpleKtBleException::throw_new(env, exception.what());
    } catch (...) {
        Kt::SimpleKtBleException::throw_new(env, "Unknown native error");
    }

    if constexpr (!std::is_void_v<ReturnType>) return ReturnType{};
}

// NOTE: Never destroyed. At JVM exit its thread cannot detach, so joining it from a static destructor hangs.
ThreadRunner& callback_runner = *new ThreadRunner();

void dispatch_scan_event(int64_t adapter_id, void (Kt::AdapterCallback::*event)() const noexcept) {
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
        throw std::runtime_error(std::string(indicate ? "An indication" : "A notification") +
                                 " collector is already active for characteristic " + characteristic);
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
        adapter.set_callback_on_scan_start(
            [adapter_id] { dispatch_scan_event(adapter_id, &Kt::AdapterCallback::on_scan_start); });
        adapter.set_callback_on_scan_stop(
            [adapter_id] { dispatch_scan_event(adapter_id, &Kt::AdapterCallback::on_scan_stop); });
        adapter.set_callback_on_scan_found(
            [adapter_id](SimpleBLE::Peripheral peripheral) { dispatch_scan_peripheral(adapter_id, peripheral, true); });
        adapter.set_callback_on_scan_updated([adapter_id](SimpleBLE::Peripheral peripheral) {
            dispatch_scan_peripheral(adapter_id, peripheral, false);
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
            callback_runner.enqueue([adapter_id, peripheral_id] {
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

            Kt::Service service_object(service.uuid(), characteristics);
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
