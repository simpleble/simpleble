#include <string>
#include <jni.h>

#include "android_utils.h"
#include <simpleble/SimpleBLE.h>
#include <simpleble/Advanced.h>
#include <simpleble/Logging.h>
#include <fmt/core.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include "ThreadRunner.h"
#include <android/log.h>

// TODO: Switch to using regular SimpleBLE classes with try/catch blocks.

static std::map<size_t, SimpleBLE::Safe::Adapter> cached_adapters;
static std::map<size_t, std::vector<jweak>> cached_adapter_callbacks;

static std::map<size_t, std::map<size_t, SimpleBLE::Safe::Peripheral>> cached_peripherals;
static std::map<size_t, std::map<size_t, std::vector<jweak>>> cached_peripheral_callbacks;
static std::map<size_t, std::map<size_t, std::map<size_t, jobject>>> cached_peripheral_data_callbacks;
static std::mutex cached_peripheral_data_callbacks_mutex;
static ThreadRunner threadRunner;
static JavaVM *jvm;

JNIEnv* get_env();

jobject get_peripheral_data_callback(JNIEnv* env, size_t adapter_id, size_t peripheral_id, size_t callback_id) {
    std::lock_guard<std::mutex> lock(cached_peripheral_data_callbacks_mutex);
    auto adapter_it = cached_peripheral_data_callbacks.find(adapter_id);
    if (adapter_it == cached_peripheral_data_callbacks.end()) return nullptr;

    auto peripheral_it = adapter_it->second.find(peripheral_id);
    if (peripheral_it == adapter_it->second.end()) return nullptr;

    auto callback_it = peripheral_it->second.find(callback_id);
    if (callback_it == peripheral_it->second.end()) return nullptr;

    return env->NewLocalRef(callback_it->second);
}

bool add_peripheral_data_callback(size_t adapter_id, size_t peripheral_id, size_t callback_id, jobject callback) {
    std::lock_guard<std::mutex> lock(cached_peripheral_data_callbacks_mutex);
    auto& callbacks = cached_peripheral_data_callbacks[adapter_id][peripheral_id];
    return callbacks.emplace(callback_id, callback).second;
}

void remove_peripheral_data_callback(JNIEnv* env, size_t adapter_id, size_t peripheral_id, size_t callback_id,
                                     jobject expected_callback = nullptr) {
    jobject callback = nullptr;
    {
        std::lock_guard<std::mutex> lock(cached_peripheral_data_callbacks_mutex);
        auto adapter_it = cached_peripheral_data_callbacks.find(adapter_id);
        if (adapter_it == cached_peripheral_data_callbacks.end()) return;

        auto peripheral_it = adapter_it->second.find(peripheral_id);
        if (peripheral_it == adapter_it->second.end()) return;

        auto callback_it = peripheral_it->second.find(callback_id);
        if (callback_it == peripheral_it->second.end()) return;
        if (expected_callback != nullptr && callback_it->second != expected_callback) return;

        callback = callback_it->second;
        peripheral_it->second.erase(callback_it);
        if (peripheral_it->second.empty()) adapter_it->second.erase(peripheral_it);
        if (adapter_it->second.empty()) cached_peripheral_data_callbacks.erase(adapter_it);
    }

    env->DeleteGlobalRef(callback);
}

void deliver_peripheral_data(size_t adapter_id, size_t peripheral_id, size_t callback_id,
                             const SimpleBLE::ByteArray& payload) {
    JNIEnv* env = get_env();
    jobject callback = get_peripheral_data_callback(env, adapter_id, peripheral_id, callback_id);
    if (callback == nullptr) return;

    jclass callback_class = env->GetObjectClass(callback);
    jmethodID on_data_received =
        callback_class == nullptr ? nullptr : env->GetMethodID(callback_class, "onDataReceived", "([B)V");
    jbyteArray j_payload = to_jbyteArray(env, payload);

    if (on_data_received != nullptr && j_payload != nullptr) {
        env->CallVoidMethod(callback, on_data_received, j_payload);
    }

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (j_payload != nullptr) env->DeleteLocalRef(j_payload);
    if (callback_class != nullptr) env->DeleteLocalRef(callback_class);
    env->DeleteLocalRef(callback);
}

JNIEnv* get_env() {
    JNIEnv *env;
    jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    return env;
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm;
    threadRunner.set_jvm(vm);
#if !SIMPLEDROIDBLE_PLAIN
    SimpleBLE::Advanced::Android::set_jvm(vm);
#endif

//    // Find your class. JNI_OnLoad is called from the correct class loader context for this to work.
//    jclass c = env->FindClass("com/example/app/package/MyClass");
//    if (c == nullptr) return JNI_ERR;

    SimpleBLE::Logging::Logger::get()->set_callback(
        [](SimpleBLE::Logging::Level level, const std::string& module, const std::string& file, uint32_t line, const std::string& function, const std::string& message) {
            std::string log_message = fmt::format("{}: {}:{} in {}: {}\n", module, file, line, function, message);

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
                    android_log_level = ANDROID_LOG_UNKNOWN;
                    break;
            }

            __android_log_write(android_log_level, "SimpleBLE", log_message.c_str());
        }
    );

    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Adapter_nativeAdapterRegister(JNIEnv *env, jobject thiz, jlong adapter_id, jobject callback) {
    // TODO: IDEA. We could store the callback object whenever the scan starts and then remove it when the scan stops,
    //             to avoid having extra references lying around.

    // Create a weak global reference to the Java callback object
    jweak weakCallbackRef = env->NewWeakGlobalRef(callback);

    // Store the weak reference in the cached_adapter_callbacks map
    cached_adapter_callbacks[adapter_id].push_back(weakCallbackRef);

    // Retrieve the adapter from the cached_adapters map
    auto adapter = cached_adapters.at(adapter_id);

    // TODO: Remove any invalid objects before adding new ones.

    adapter.set_callback_on_scan_start([adapter_id](){
        threadRunner.enqueue([adapter_id](){
            JNIEnv *env = get_env();

            // Retrieve the weak references from the cached_adapter_callbacks map
            std::vector<jweak> weakCallbackRefs = cached_adapter_callbacks[adapter_id];

            // Iterate over the weak references
            for (jweak weakCallbackRef : weakCallbackRefs) {

                // Check if the weak reference is still valid
                if (env->IsSameObject(weakCallbackRef, nullptr) == JNI_FALSE) {
                    // Retrieve the strong reference from the weak reference
                    jobject callbackRef = env->NewLocalRef(weakCallbackRef);
                    if (callbackRef == nullptr) continue;

                    // Find the Java class and method to invoke
                    // TODO: We should cache the class and method IDs
                    jclass callbackClass = env->GetObjectClass(callbackRef);
                    jmethodID onScanStartMethod = env->GetMethodID(callbackClass, "onScanStart", "()V");

                    // Invoke the Java callback method
                    env->CallVoidMethod(callbackRef, onScanStartMethod);

                    // Delete the local reference
                    env->DeleteLocalRef(callbackClass);
                    env->DeleteLocalRef(callbackRef);
                }
            }
        });
    });

    adapter.set_callback_on_scan_stop([adapter_id](){
        threadRunner.enqueue([adapter_id](){
            JNIEnv *env = get_env();

            // Retrieve the weak references from the cached_adapter_callbacks map
            std::vector<jweak> weakCallbackRefs = cached_adapter_callbacks[adapter_id];

            // Iterate over the weak references
            for (jweak weakCallbackRef : weakCallbackRefs) {
                // Check if the weak reference is still valid
                if (env->IsSameObject(weakCallbackRef, nullptr) == JNI_FALSE) {
                    // Retrieve the strong reference from the weak reference
                    jobject callbackRef = env->NewLocalRef(weakCallbackRef);
                    if (callbackRef == nullptr) continue;

                    // Find the Java class and method to invoke
                    // TODO: We should cache the class and method IDs
                    jclass callbackClass = env->GetObjectClass(callbackRef);
                    jmethodID onScanStopMethod = env->GetMethodID(callbackClass, "onScanStop", "()V");

                    // Invoke the Java callback method
                    env->CallVoidMethod(callbackRef, onScanStopMethod);

                    // Delete the local reference
                    env->DeleteLocalRef(callbackClass);
                    env->DeleteLocalRef(callbackRef);
                }
            }
        });
    });

    adapter.set_callback_on_scan_found([adapter_id](SimpleBLE::Safe::Peripheral peripheral){
        size_t peripheral_hash = std::hash<std::string>{}(peripheral.address().value_or("UNKNOWN"));

        // Add to the cache if it doesn't exist
        if (cached_peripherals[adapter_id].count(peripheral_hash) == 0) {
            cached_peripherals[adapter_id].insert({peripheral_hash, peripheral});
        }

        threadRunner.enqueue([adapter_id, peripheral_hash](){
            JNIEnv *env = get_env();

            // Retrieve the weak references from the cached_adapter_callbacks map
            std::vector<jweak> weakCallbackRefs = cached_adapter_callbacks[adapter_id];

            // Iterate over the weak references
            for (jweak weakCallbackRef : weakCallbackRefs) {
                // Check if the weak reference is still valid
                if (env->IsSameObject(weakCallbackRef, nullptr) == JNI_FALSE) {

                    // Retrieve the strong reference from the weak reference
                    jobject callbackRef = env->NewLocalRef(weakCallbackRef);
                    if (callbackRef == nullptr) continue;

                    // Find the Java class and method to invoke
                    // TODO: We should cache the class and method IDs
                    jclass callbackClass = env->GetObjectClass(callbackRef);
                    jmethodID onScanFoundMethod = env->GetMethodID(callbackClass, "onScanFound","(J)V");

                    // Invoke the Java callback method
                    env->CallVoidMethod(callbackRef, onScanFoundMethod, peripheral_hash);

                    // Delete the local reference
                    env->DeleteLocalRef(callbackClass);
                    env->DeleteLocalRef(callbackRef);

                }
            }
        });
    });

    adapter.set_callback_on_scan_updated([adapter_id](SimpleBLE::Safe::Peripheral peripheral){
        size_t peripheral_hash = std::hash<std::string>{}(peripheral.address().value_or("UNKNOWN"));

        // Add to the cache if it doesn't exist
        if (cached_peripherals[adapter_id].count(peripheral_hash) == 0) {
            cached_peripherals[adapter_id].insert({peripheral_hash, peripheral});
        }

        threadRunner.enqueue([adapter_id, peripheral_hash](){
            JNIEnv *env = get_env();

            // Retrieve the weak references from the cached_adapter_callbacks map
            std::vector<jweak> weakCallbackRefs = cached_adapter_callbacks[adapter_id];

            // Iterate over the weak references
            for (jweak weakCallbackRef : weakCallbackRefs) {
                // Check if the weak reference is still valid
                if (env->IsSameObject(weakCallbackRef, nullptr) == JNI_FALSE) {

                    // Retrieve the strong reference from the weak reference
                    jobject callbackRef = env->NewLocalRef(weakCallbackRef);
                    if (callbackRef == nullptr) continue;

                    // Find the Java class and method to invoke
                    // TODO: We should cache the class and method IDs
                    jclass callbackClass = env->GetObjectClass(callbackRef);
                    jmethodID onScanFoundMethod = env->GetMethodID(callbackClass, "onScanUpdated", "(J)V");

                    // Invoke the Java callback method
                    env->CallVoidMethod(callbackRef, onScanFoundMethod, peripheral_hash);

                    // Delete the local reference
                    env->DeleteLocalRef(callbackClass);
                    env->DeleteLocalRef(callbackRef);

                }
            }
        });
    });
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_org_simpleble_android_Adapter_00024Companion_nativeIsBluetoothEnabled(JNIEnv *env, jobject thiz) {
    auto enabled = SimpleBLE::Safe::Adapter::bluetooth_enabled();
    if (!enabled.has_value()) {
        throw_exception(env, "Failed to read Bluetooth adapter state");
        return false;
    }
    return enabled.value();
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simpleble_android_Adapter_nativeGetAdapters(JNIEnv *env, jclass clazz) {
    auto adapters = SimpleBLE::Safe::Adapter::get_adapters();

    if (!adapters.has_value()) {
        throw_exception(env, "Failed to get Bluetooth adapters");
        return nullptr;
    }

    // Go over the results, cache whatever doesn't exist and return the full list.
    jsize j_adapter_index = 0;
    jlongArray j_adapter_result = env->NewLongArray(static_cast<int>(adapters.value().size()));
    for (auto &adapter: adapters.value()) {
        size_t adapter_hash = std::hash<std::string>{}(adapter.identifier().value_or("UNKNOWN"));

        // Add to the cache if it doesn't exist
        if (cached_adapters.count(adapter_hash) == 0) {
            cached_adapters.insert({adapter_hash, adapter});
        }

        // Add to the results
        jlong j_adapter_hash = adapter_hash;
        env->SetLongArrayRegion(j_adapter_result, j_adapter_index, 1, &j_adapter_hash);
        j_adapter_index++;

    }

    return j_adapter_result;
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simpleble_android_Adapter_nativeAdapterIdentifier(JNIEnv *env, jobject thiz, jlong adapter_id) {
    auto adapter = cached_adapters.at(adapter_id);
    auto identifier = adapter.identifier();
    if (!identifier.has_value()) {
        throw_exception(env, "Failed to get adapter identifier");
        return nullptr;
    }
    return to_jstring(env, identifier.value());
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simpleble_android_Adapter_nativeAdapterAddress(JNIEnv *env, jobject thiz, jlong adapter_id) {
    auto adapter = cached_adapters.at(adapter_id);
    auto address = adapter.address();
    if (!address.has_value()) {
        throw_exception(env, "Failed to get adapter address");
        return nullptr;
    }
    return to_jstring(env, address.value());
}

extern "C" JNIEXPORT void JNICALL Java_org_simpleble_android_Adapter_nativeAdapterScanStart(JNIEnv *env, jobject thiz, jlong adapter_id) {
    auto adapter = cached_adapters.at(adapter_id);
    bool success = adapter.scan_start();

    if (!success) {
        throw_exception(env, "Failed to start scan");
    }
}

extern "C" JNIEXPORT void JNICALL Java_org_simpleble_android_Adapter_nativeAdapterScanStop(JNIEnv *env, jobject thiz, jlong adapter_id) {
    auto adapter = cached_adapters.at(adapter_id);
    bool success = adapter.scan_stop();

    if (!success) {
        throw_exception(env, "Failed to stop scan");
    }
}

extern "C" JNIEXPORT void JNICALL Java_org_simpleble_android_Adapter_nativeAdapterScanFor(JNIEnv *env, jobject thiz, jlong adapter_id, jint timeout) {
    auto adapter = cached_adapters.at(adapter_id);
    bool success = adapter.scan_for(timeout);

    if (!success) {
        throw_exception(env, "Failed to scan for");
    }
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simpleble_android_Adapter_nativeAdapterScanIsActive(JNIEnv *env, jobject thiz, jlong adapter_id) {
    auto adapter = cached_adapters.at(adapter_id);
    auto active = adapter.scan_is_active();
    if (!active.has_value()) {
        throw_exception(env, "Failed to get scan state");
        return false;
    }
    return active.value();
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simpleble_android_Adapter_nativeAdapterScanGetResults(JNIEnv *env, jobject thiz, jlong adapter_id) {
    auto adapter = cached_adapters.at(adapter_id);

    auto peripherals = adapter.scan_get_results();

    if (!peripherals.has_value()) {
        throw_exception(env, "Failed to get scan results");
        return nullptr;
    }

    jsize j_peripheral_index = 0;
    jlongArray j_peripheral_result = env->NewLongArray(static_cast<int>(peripherals.value().size()));
    for (auto &peripheral: peripherals.value()) {
        size_t peripheral_hash = std::hash<std::string>{}(peripheral.address().value_or("UNKNOWN"));

        // Add to the cache if it doesn't exist
        if (cached_peripherals[adapter_id].count(peripheral_hash) == 0) {
            cached_peripherals[adapter_id].insert({peripheral_hash, peripheral});
        }

        // Add to the results
        jlong j_peripheral_hash = peripheral_hash;
        env->SetLongArrayRegion(j_peripheral_result, j_peripheral_index, 1, &j_peripheral_hash);
        j_peripheral_index++;
    }

    return j_peripheral_result;
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simpleble_android_Adapter_nativeAdapterGetPairedPeripherals(JNIEnv *env, jobject thiz, jlong adapter_id) {
    auto adapter = cached_adapters.at(adapter_id);
    auto peripherals = adapter.get_paired_peripherals();

    if (!peripherals.has_value()) {
        throw_exception(env, "Failed to get paired peripherals");
        return nullptr;
    }

    jsize j_peripheral_index = 0;
    jlongArray j_peripheral_result = env->NewLongArray(static_cast<int>(peripherals.value().size()));
    for (auto &peripheral: peripherals.value()) {
        size_t peripheral_hash = std::hash<std::string>{}(peripheral.address().value_or("UNKNOWN"));

        if (cached_peripherals[adapter_id].count(peripheral_hash) == 0) {
            cached_peripherals[adapter_id].insert({peripheral_hash, peripheral});
        }

        jlong j_peripheral_hash = peripheral_hash;
        env->SetLongArrayRegion(j_peripheral_result, j_peripheral_index, 1, &j_peripheral_hash);
        j_peripheral_index++;
    }

    return j_peripheral_result;
}

// PERIPHERAL

extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralRegister(JNIEnv *env, jobject thiz,
                                                               jlong adapter_id, jlong peripheral_id, jobject callback) {
// TODO: IDEA. We could store the callback object whenever the scan starts and then remove it when the scan stops,
    //             to avoid having extra references lying around.

    // Create a weak global reference to the Java callback object
    jweak weakCallbackRef = env->NewWeakGlobalRef(callback);

    // Store the weak reference in the cached_adapter_callbacks map
    cached_peripheral_callbacks[adapter_id][peripheral_id].push_back(weakCallbackRef);

    auto peripheral = cached_peripherals[adapter_id].at(peripheral_id);

    // TODO: Remove any invalid objects before adding new ones.

    peripheral.set_callback_on_connected([adapter_id, peripheral_id](){
        threadRunner.enqueue([adapter_id, peripheral_id](){
            JNIEnv *env = get_env();

            // Retrieve the weak references from the cached_adapter_callbacks map
            std::vector<jweak> weakCallbackRefs = cached_peripheral_callbacks[adapter_id][peripheral_id];

            // Iterate over the weak references
            for (jweak weakCallbackRef : weakCallbackRefs) {

                // Check if the weak reference is still valid
                if (env->IsSameObject(weakCallbackRef, nullptr) == JNI_FALSE) {
                    // Retrieve the strong reference from the weak reference
                    jobject callbackRef = env->NewLocalRef(weakCallbackRef);
                    if (callbackRef == nullptr) continue;

                    // Find the Java class and method to invoke
                    // TODO: We should cache the class and method IDs
                    jclass callbackClass = env->GetObjectClass(callbackRef);
                    jmethodID onConnectedMethod = env->GetMethodID(callbackClass, "onConnected", "()V");

                    // Invoke the Java callback method
                    env->CallVoidMethod(callbackRef, onConnectedMethod);

                    // Delete the local reference
                    env->DeleteLocalRef(callbackClass);
                    env->DeleteLocalRef(callbackRef);
                }
            }
        });
    });

    peripheral.set_callback_on_disconnected([adapter_id, peripheral_id](){
        threadRunner.enqueue([adapter_id, peripheral_id](){
            JNIEnv *env = get_env();

            // Retrieve the weak references from the cached_adapter_callbacks map
            std::vector<jweak> weakCallbackRefs = cached_peripheral_callbacks[adapter_id][peripheral_id];

            // Iterate over the weak references
            for (jweak weakCallbackRef : weakCallbackRefs) {

                // Check if the weak reference is still valid
                if (env->IsSameObject(weakCallbackRef, nullptr) == JNI_FALSE) {
                    // Retrieve the strong reference from the weak reference
                    jobject callbackRef = env->NewLocalRef(weakCallbackRef);
                    if (callbackRef == nullptr) continue;

                    // Find the Java class and method to invoke
                    // TODO: We should cache the class and method IDs
                    jclass callbackClass = env->GetObjectClass(callbackRef);
                    jmethodID onConnectedMethod = env->GetMethodID(callbackClass, "onDisconnected", "()V");

                    // Invoke the Java callback method
                    env->CallVoidMethod(callbackRef, onConnectedMethod);

                    // Delete the local reference
                    env->DeleteLocalRef(callbackClass);
                    env->DeleteLocalRef(callbackRef);
                }
            }
        });
    });
}

extern "C"
JNIEXPORT jstring JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralIdentifier(JNIEnv *env, jobject thiz,
                                                                 jlong adapter_id,
                                                                 jlong peripheral_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    auto identifier = peripheral.identifier();
    if (!identifier.has_value()) {
        throw_exception(env, "Failed to get peripheral identifier");
        return nullptr;
    }
    return to_jstring(env, identifier.value());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralAddress(JNIEnv *env, jobject thiz,
                                                              jlong adapter_id, jlong peripheral_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    auto address = peripheral.address();
    if (!address.has_value()) {
        throw_exception(env, "Failed to get peripheral address");
        return nullptr;
    }
    return to_jstring(env, address.value());
}

extern "C"
JNIEXPORT jint JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralAddressType(JNIEnv *env, jobject thiz,
                                                                  jlong adapter_id,
                                                                  jlong peripheral_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    auto address_type = peripheral.address_type();
    if (!address_type.has_value()) {
        throw_exception(env, "Failed to get peripheral address type");
        return SimpleBLE::BluetoothAddressType::UNSPECIFIED;
    }
    return address_type.value();
}

extern "C"
JNIEXPORT jint JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralRssi(JNIEnv *env, jobject thiz,
                                                           jlong adapter_id, jlong peripheral_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    auto rssi = peripheral.rssi();
    if (!rssi.has_value()) {
        throw_exception(env, "Failed to get peripheral RSSI");
        return 0;
    }
    return rssi.value();
}

extern "C"
JNIEXPORT jint JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralTxPower(JNIEnv *env, jobject thiz,
                                                              jlong adapter_id, jlong peripheral_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    auto tx_power = peripheral.tx_power();
    if (!tx_power.has_value()) {
        throw_exception(env, "Failed to get peripheral transmit power");
        return 0;
    }
    return tx_power.value();
}

extern "C"
JNIEXPORT jint JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralMtu(JNIEnv *env, jobject thiz,
                                                          jlong adapter_id, jlong peripheral_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    auto mtu = peripheral.mtu();
    if (!mtu.has_value()) {
        throw_exception(env, "Failed to get peripheral MTU");
        return 0;
    }
    return mtu.value();
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralConnect(JNIEnv *env, jobject thiz,
                                                              jlong adapter_id, jlong peripheral_id) {
    auto peripheral = cached_peripherals[adapter_id].at(peripheral_id);

    bool success = peripheral.connect();
    if (!success) {
        throw_exception(env, "Failed to connect");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralDisconnect(JNIEnv *env, jobject thiz,
                                                                 jlong adapter_id,
                                                                 jlong peripheral_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    if (!peripheral.disconnect()) {
        throw_exception(env, "Failed to disconnect");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralNotify(JNIEnv *env, jobject thiz,
                                                             jlong adapter_id, jlong peripheral_id,
                                                             jstring j_service,
                                                             jstring j_characteristic,
                                                             jobject callback) {

    std::string service = from_jstring(env, j_service);
    std::string characteristic = from_jstring(env, j_characteristic);
    std::string service_characteristic = service + "_" + characteristic;
    size_t service_characteristic_hash = std::hash<std::string>{}(service_characteristic);

    jobject callbackRef = env->NewGlobalRef(callback);
    if (callbackRef == nullptr) return;
    if (!add_peripheral_data_callback(adapter_id, peripheral_id, service_characteristic_hash, callbackRef)) {
        env->DeleteGlobalRef(callbackRef);
        throw_exception(env, "A notification collector is already active for characteristic " + characteristic);
        return;
    }

    auto peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    bool success = peripheral.notify(service, characteristic, [adapter_id, peripheral_id, service_characteristic_hash](SimpleBLE::ByteArray payload){
        threadRunner.enqueue([adapter_id, peripheral_id, service_characteristic_hash, payload]() {
            deliver_peripheral_data(adapter_id, peripheral_id, service_characteristic_hash, payload);
        });
    });

    if (!success) {
        remove_peripheral_data_callback(env, adapter_id, peripheral_id, service_characteristic_hash, callbackRef);
        throw_exception(env, "Failed to notify");
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralIndicate(JNIEnv *env, jobject thiz,
                                                               jlong adapter_id, jlong peripheral_id,
                                                               jstring j_service,
                                                               jstring j_characteristic,
                                                               jobject callback) {

    std::string service = from_jstring(env, j_service);
    std::string characteristic = from_jstring(env, j_characteristic);
    std::string service_characteristic = service + "_" + characteristic;
    size_t service_characteristic_hash = std::hash<std::string>{}(service_characteristic);

    jobject callbackRef = env->NewGlobalRef(callback);
    if (callbackRef == nullptr) return;
    if (!add_peripheral_data_callback(adapter_id, peripheral_id, service_characteristic_hash, callbackRef)) {
        env->DeleteGlobalRef(callbackRef);
        throw_exception(env, "An indication collector is already active for characteristic " + characteristic);
        return;
    }

    auto peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    bool success = peripheral.indicate(service, characteristic, [adapter_id, peripheral_id, service_characteristic_hash](SimpleBLE::ByteArray payload){
        threadRunner.enqueue([adapter_id, peripheral_id, service_characteristic_hash, payload]() {
            deliver_peripheral_data(adapter_id, peripheral_id, service_characteristic_hash, payload);
        });
    });

    if (!success) {
        remove_peripheral_data_callback(env, adapter_id, peripheral_id, service_characteristic_hash, callbackRef);
        throw_exception(env, "Failed to indicate");
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralUnsubscribe(JNIEnv *env, jobject thiz,
                                                                  jlong adapter_id,
                                                                  jlong peripheral_id,
                                                                  jstring j_service,
                                                                  jstring j_characteristic) {
    std::string service = from_jstring(env, j_service);
    std::string characteristic = from_jstring(env, j_characteristic);
    std::string service_characteristic = service + "_" + characteristic;
    size_t service_characteristic_hash = std::hash<std::string>{}(service_characteristic);

    auto peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    bool success = peripheral.unsubscribe(service, characteristic);

    remove_peripheral_data_callback(env, adapter_id, peripheral_id, service_characteristic_hash);

    if (!success) throw_exception(env, "Failed to unsubscribe");
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralIsConnected(JNIEnv *env, jobject thiz,
                                                                  jlong adapter_id,
                                                                  jlong instance_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(instance_id);
    auto connected = peripheral.is_connected();
    if (!connected.has_value()) {
        throw_exception(env, "Failed to get connection state");
        return false;
    }
    return connected.value();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralIsConnectable(JNIEnv *env, jobject thiz,
                                                                    jlong adapter_id,
                                                                    jlong instance_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(instance_id);
    auto connectable = peripheral.is_connectable();
    if (!connectable.has_value()) {
        throw_exception(env, "Failed to get connectable state");
        return false;
    }
    return connectable.value();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralIsPaired(JNIEnv *env, jobject thiz,
                                                               jlong adapter_id,
                                                               jlong instance_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(instance_id);
    auto paired = peripheral.is_paired();
    if (!paired.has_value()) {
        throw_exception(env, "Failed to get paired state");
        return false;
    }
    return paired.value();
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralUnpair(JNIEnv *env, jobject thiz,
                                                             jlong adapter_id, jlong instance_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(instance_id);
    peripheral.unpair();
}

extern "C"
JNIEXPORT jobject JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralServices(JNIEnv* env, jobject thiz,
                                                               jlong adapter_id,
                                                               jlong peripheral_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    auto services = peripheral.services();
    if (!services.has_value()) {
        throw_exception(env, "Failed to get peripheral services");
        return nullptr;
    }

    jclass serviceClass = env->FindClass("org/simpleble/android/Service");
    jclass characteristicClass = env->FindClass("org/simpleble/android/Characteristic");
    jclass descriptorClass = env->FindClass("org/simpleble/android/Descriptor");

    jmethodID serviceConstructor = env->GetMethodID(serviceClass, "<init>", "(Ljava/lang/String;Ljava/util/List;)V");
    jmethodID characteristicConstructor = env->GetMethodID(characteristicClass, "<init>", "(Ljava/lang/String;Ljava/util/List;ZZZZZ)V");
    jmethodID descriptorConstructor = env->GetMethodID(descriptorClass, "<init>", "(Ljava/lang/String;)V");

    jobject serviceArray = jarraylist_new(env);

    for (auto service : services.value()) {
        jstring serviceUUID = to_jstring(env, service.uuid());
        jobject charList = jarraylist_new(env);

        for (auto characteristic : service.characteristics()) {
            jstring charUUID = to_jstring(env, characteristic.uuid());
            jobject descList = jarraylist_new(env);

            for (auto descriptor : characteristic.descriptors()) {
                jstring descUUID = to_jstring(env, descriptor.uuid());
                jobject jDescriptor = env->NewObject(descriptorClass, descriptorConstructor, descUUID);
                jarraylist_add(env, descList, jDescriptor);
                env->DeleteLocalRef(jDescriptor);
                env->DeleteLocalRef(descUUID);
            }

            jobject jCharacteristic = env->NewObject(characteristicClass, characteristicConstructor,
                                                     charUUID, descList,
                                                     characteristic.can_read(),
                                                     characteristic.can_write_request(),
                                                     characteristic.can_write_command(),
                                                     characteristic.can_notify(),
                                                     characteristic.can_indicate());
            jarraylist_add(env, charList, jCharacteristic);
            env->DeleteLocalRef(jCharacteristic);
            env->DeleteLocalRef(descList);
            env->DeleteLocalRef(charUUID);
        }

        jobject jService = env->NewObject(serviceClass, serviceConstructor, serviceUUID, charList);
        jarraylist_add(env, serviceArray, jService);
        env->DeleteLocalRef(jService);
        env->DeleteLocalRef(charList);
        env->DeleteLocalRef(serviceUUID);
    }

    env->DeleteLocalRef(descriptorClass);
    env->DeleteLocalRef(characteristicClass);
    env->DeleteLocalRef(serviceClass);

    return serviceArray;
}

// Utility function to create a new HashMap and return it
jobject NewHashMap(JNIEnv* env) {
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (hashMapClass == nullptr) {
        return nullptr; // Class not found
    }
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (hashMapConstructor == nullptr) {
        env->DeleteLocalRef(hashMapClass);
        return nullptr; // Constructor method not found
    }
    jobject hashMap = env->NewObject(hashMapClass, hashMapConstructor);
    env->DeleteLocalRef(hashMapClass);
    return hashMap;
}

// Utility function to add an entry to a HashMap
void HashMapPut(JNIEnv* env, jobject hashMap, jobject key, jobject value) {
    jclass hashMapClass = env->GetObjectClass(hashMap);
    if (hashMapClass == nullptr) {
        return; // Class not found
    }
    jmethodID hashMapPut = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (hashMapPut == nullptr) {
        env->DeleteLocalRef(hashMapClass);
        return; // Method not found
    }
    jobject previousValue = env->CallObjectMethod(hashMap, hashMapPut, key, value);
    if (previousValue != nullptr) env->DeleteLocalRef(previousValue);
    env->DeleteLocalRef(hashMapClass);
}


// Convert a C++ int to a Java Integer
jobject to_jInteger(JNIEnv* env, jint value) {
    jclass integerClass = env->FindClass("java/lang/Integer");
    if (!integerClass) return nullptr;  // Class not found

    jmethodID integerConstructor = env->GetMethodID(integerClass, "<init>", "(I)V");
    if (!integerConstructor) {
        env->DeleteLocalRef(integerClass);
        return nullptr;  // Constructor method not found
    }

    jobject integerObject = env->NewObject(integerClass, integerConstructor, value);
    env->DeleteLocalRef(integerClass);
    return integerObject;
}

// JNI function implementation
extern "C"
JNIEXPORT jobject JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralManufacturerData(JNIEnv* env, jobject thiz, jlong adapter_id, jlong instance_id) {
    auto& peripheral = cached_peripherals[adapter_id].at(instance_id);
    auto manufacturer_data = peripheral.manufacturer_data();
    if (!manufacturer_data.has_value()) {
        throw_exception(env, "Failed to get manufacturer data");
        return nullptr;
    }

    jobject hashMap = NewHashMap(env);
    if (!hashMap) return nullptr;  // Error creating HashMap

    for (const auto& data : manufacturer_data.value()) {
        jobject key = to_jInteger(env, static_cast<jint>(data.first));
        jbyteArray value = to_jbyteArray(env, data.second);

        HashMapPut(env, hashMap, key, value);

        env->DeleteLocalRef(key);
        env->DeleteLocalRef(value);
    }

    return hashMap;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralRead(JNIEnv *env, jobject thiz,
                                                           jlong adapter_id, jlong peripheral_id,
                                                           jstring j_service,
                                                           jstring j_characteristic) {
    std::string service = from_jstring(env, j_service);
    std::string characteristic = from_jstring(env, j_characteristic);

    auto peripheral = cached_peripherals[adapter_id].at(peripheral_id);
    auto result = peripheral.read(service, characteristic);
    if (!result.has_value()) {
        throw_exception(env, "Failed to read characteristic " + characteristic);
        return nullptr;
    }

    return to_jbyteArray(env, result.value());
}
extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralWriteRequest(JNIEnv *env, jobject thiz,
                                                                   jlong adapter_id,
                                                                   jlong instance_id,
                                                                   jstring j_service,
                                                                   jstring j_characteristic,
                                                                   jbyteArray data) {
    std::string service = from_jstring(env, j_service);
    std::string characteristic = from_jstring(env, j_characteristic);
    auto peripheral = cached_peripherals[adapter_id].at(instance_id);
    bool success = peripheral.write_request(service, characteristic, SimpleBLE::ByteArray(from_jbyteArray(env, data)));

    if (!success) {
        throw_exception(env, "Failed to write request to characteristic " + characteristic);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralWriteCommand(JNIEnv *env, jobject thiz,
                                                                   jlong adapter_id,
                                                                   jlong instance_id,
                                                                   jstring j_service,
                                                                   jstring j_characteristic,
                                                                   jbyteArray data) {
    std::string service = from_jstring(env, j_service);
    std::string characteristic = from_jstring(env, j_characteristic);
    auto peripheral = cached_peripherals[adapter_id].at(instance_id);
    bool success = peripheral.write_command(service, characteristic, SimpleBLE::ByteArray(from_jbyteArray(env, data)));

    if (!success) {
        throw_exception(env, "Failed to write command to characteristic " + characteristic);
    }
}
extern "C"
JNIEXPORT jbyteArray JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralDescriptorRead(JNIEnv *env, jobject thiz,
                                                                     jlong adapter_id,
                                                                     jlong instance_id,
                                                                     jstring j_service,
                                                                     jstring j_characteristic,
                                                                     jstring j_descriptor) {
    std::string service = from_jstring(env, j_service);
    std::string characteristic = from_jstring(env, j_characteristic);
    std::string descriptor = from_jstring(env, j_descriptor);

    auto peripheral = cached_peripherals[adapter_id].at(instance_id);
    auto result = peripheral.read(service, characteristic, descriptor);
    if (!result.has_value()) {
        throw_exception(env, "Failed to read descriptor " + descriptor);
        return nullptr;
    }

    return to_jbyteArray(env, result.value());
}
extern "C"
JNIEXPORT void JNICALL
Java_org_simpleble_android_Peripheral_nativePeripheralDescriptorWrite(JNIEnv *env, jobject thiz,
                                                                      jlong adapter_id,
                                                                      jlong instance_id,
                                                                      jstring j_service,
                                                                      jstring j_characteristic,
                                                                      jstring j_descriptor,
                                                                      jbyteArray data) {
    std::string service = from_jstring(env, j_service);
    std::string characteristic = from_jstring(env, j_characteristic);
    std::string descriptor = from_jstring(env, j_descriptor);

    auto peripheral = cached_peripherals[adapter_id].at(instance_id);
    bool success = peripheral.write(service, characteristic, descriptor, SimpleBLE::ByteArray(from_jbyteArray(env, data)));

    if (!success) {
        throw_exception(env, "Failed to write descriptor " + descriptor);
    }
}
