#include <string>
#include <jni.h>

#include "utils.h"
#include <simpleble/SimpleBLE.h>
#include <simpleble/Logging.h>
#include <fmt/core.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include "ThreadRunner.h"

#include "core/Cache.h"
#include "core/AdapterWrapper.h"
#include "jni/Common.hpp"

using namespace SimpleJNI;

// TODO: Switch to using regular SimpleBLE classes with try/catch blocks.

static std::map<size_t, SimpleBLE::Safe::Adapter> cached_adapters;
static std::map<size_t, std::vector<jweak>> cached_adapter_callbacks;

static std::map<size_t, std::map<size_t, SimpleBLE::Safe::Peripheral>> cached_peripherals;
static std::map<size_t, std::map<size_t, std::vector<jweak>>> cached_peripheral_callbacks;
static std::map<size_t, std::map<size_t, std::map<size_t, jobject>>> cached_peripheral_data_callbacks;
static ThreadRunner threadRunner;
static JavaVM *jvm;

JNIEnv* get_env() {
    JNIEnv *env;
    jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    return env;
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm;
    threadRunner.set_jvm(vm);

    return JNI_VERSION_1_6;
}





extern "C" JNIEXPORT
jboolean JNICALL Java_org_simplejavable_Adapter_00024Companion_nativeIsBluetoothEnabled(JNIEnv *env, jobject thiz) {
    //return SimpleBLE::Safe::Adapter::bluetooth_enabled().value_or(false);
    return true;
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Adapter_nativeGetAdapters(JNIEnv *env, jclass clazz) {
    std::vector<SimpleBLE::Adapter> adapters = SimpleBLE::Adapter::get_adapters();
    if (adapters.empty()) return env->NewLongArray(0);

    std::vector<int64_t> adapter_hashes;
    for (SimpleBLE::Adapter &adapter: adapters) {
        AdapterWrapper adapter_wrapper(adapter);
        adapter_hashes.push_back(adapter_wrapper.getHash());
        Cache::get().addAdapter(adapter_wrapper.getHash(), adapter_wrapper);
    }

    LongArray<LocalRef> adapter_hashes_array(adapter_hashes);
    return adapter_hashes_array.get();
}

extern "C" JNIEXPORT
void JNICALL Java_org_simplejavable_Adapter_nativeAdapterRegister(JNIEnv *env, jobject thiz, jlong adapter_id, jobject callback) {
    // TODO: IDEA. We could store the callback object whenever the scan starts and then remove it when the scan stops,
    //             to avoid having extra references lying around.

    // Create a weak global reference to the Java callback object
    // jweak weakCallbackRef = env->NewWeakGlobalRef(callback);

    // // Store the weak reference in the cached_adapter_callbacks map
    // cached_adapter_callbacks[adapter_id].push_back(weakCallbackRef);

    // // Retrieve the adapter from the cached_adapters map
    // auto adapter = cached_adapters.at(adapter_id);

    // TODO: Remove any invalid objects before adding new ones.


}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Adapter_nativeAdapterIdentifier(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    return String<LocalRef>(adapter_wrapper->identifier()).get();
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Adapter_nativeAdapterAddress(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    return String<LocalRef>(adapter_wrapper->address()).get();
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanStart(JNIEnv *env, jobject thiz, jlong adapter_id) {
    // auto adapter = cached_adapters.at(adapter_id);
    // bool success = adapter.scan_start();

    // if (!success) {
    //     throw_exception(env, "Failed to start scan");
    // }
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanStop(JNIEnv *env, jobject thiz, jlong adapter_id) {
    // auto adapter = cached_adapters.at(adapter_id);
    // bool success = adapter.scan_stop();

    // if (!success) {
    //     throw_exception(env, "Failed to stop scan");
    // }
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanFor(JNIEnv *env, jobject thiz, jlong adapter_id, jint timeout) {
//     auto adapter = cached_adapters.at(adapter_id);
//     bool success = adapter.scan_for(timeout);

//     if (!success) {
//         throw_exception(env, "Failed to scan for");
//     }
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanIsActive(JNIEnv *env, jobject thiz, jlong adapter_id) {
    // auto adapter = cached_adapters.at(adapter_id);
    // // TODO: Should throw exception in case of failure.
    // return adapter.scan_is_active().value_or(false);
    return JNI_FALSE;
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanGetResults(JNIEnv *env, jobject thiz, jlong adapter_id) {
    // auto adapter = cached_adapters.at(adapter_id);

    // auto peripherals = adapter.scan_get_results();

    // // If an error occurred, return an empty list.
    // if (!peripherals.has_value()) return env->NewLongArray(0);

    // jsize j_peripheral_index = 0;
    // jlongArray j_peripheral_result = env->NewLongArray(static_cast<int>(peripherals.value().size()));
    // for (auto &peripheral: peripherals.value()) {
    //     size_t peripheral_hash = std::hash<std::string>{}(peripheral.address().value_or("UNKNOWN"));

    //     // Add to the cache if it doesn't exist
    //     if (cached_peripherals[adapter_id].count(peripheral_hash) == 0) {
    //         cached_peripherals[adapter_id].insert({peripheral_hash, peripheral});
    //     }

    //     // Add to the results
    //     jlong j_peripheral_hash = peripheral_hash;
    //     env->SetLongArrayRegion(j_peripheral_result, j_peripheral_index, 1, &j_peripheral_hash);
    //     j_peripheral_index++;
    // }

    // return j_peripheral_result;
}

// PERIPHERAL

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralRegister(JNIEnv *env, jobject thiz,
                                                          jlong adapter_id, jlong peripheral_id, jobject callback) {
}

extern "C"
JNIEXPORT jstring JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralIdentifier(JNIEnv *env, jobject thiz,
                                                            jlong adapter_id, jlong peripheral_id) {
    return nullptr;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralAddress(JNIEnv *env, jobject thiz,
                                                         jlong adapter_id, jlong peripheral_id) {
    return nullptr;
}

extern "C"
JNIEXPORT jint JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralAddressType(JNIEnv *env, jobject thiz,
                                                             jlong adapter_id, jlong peripheral_id) {
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralRssi(JNIEnv *env, jobject thiz,
                                                      jlong adapter_id, jlong peripheral_id) {
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralTxPower(JNIEnv *env, jobject thiz,
                                                         jlong adapter_id, jlong peripheral_id) {
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralMtu(JNIEnv *env, jobject thiz,
                                                     jlong adapter_id, jlong peripheral_id) {
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralConnect(JNIEnv *env, jobject thiz,
                                                         jlong adapter_id, jlong peripheral_id) {
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralDisconnect(JNIEnv *env, jobject thiz,
                                                            jlong adapter_id, jlong peripheral_id) {
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralNotify(JNIEnv *env, jobject thiz,
                                                        jlong adapter_id, jlong peripheral_id,
                                                        jstring j_service, jstring j_characteristic,
                                                        jobject callback) {
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralIndicate(JNIEnv *env, jobject thiz,
                                                          jlong adapter_id, jlong peripheral_id,
                                                          jstring j_service, jstring j_characteristic,
                                                          jobject callback) {
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralUnsubscribe(JNIEnv *env, jobject thiz,
                                                             jlong adapter_id, jlong peripheral_id,
                                                             jstring j_service, jstring j_characteristic) {
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralIsConnected(JNIEnv *env, jobject thiz,
                                                             jlong adapter_id, jlong instance_id) {
    return JNI_FALSE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralIsConnectable(JNIEnv *env, jobject thiz,
                                                               jlong adapter_id, jlong instance_id) {
    return JNI_FALSE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralIsPaired(JNIEnv *env, jobject thiz,
                                                          jlong adapter_id, jlong instance_id) {
    return JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralUnpair(JNIEnv *env, jobject thiz,
                                                        jlong adapter_id, jlong instance_id) {
}

extern "C"
JNIEXPORT jobject JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralServices(JNIEnv* env, jobject thiz,
                                                          jlong adapter_id, jlong peripheral_id) {
    return nullptr;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralManufacturerData(JNIEnv* env, jobject thiz,
                                                                  jlong adapter_id, jlong instance_id) {
    return nullptr;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralRead(JNIEnv *env, jobject thiz,
                                                      jlong adapter_id, jlong peripheral_id,
                                                      jstring j_service, jstring j_characteristic) {
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralWriteRequest(JNIEnv *env, jobject thiz,
                                                              jlong adapter_id, jlong instance_id,
                                                              jstring service, jstring characteristic,
                                                              jbyteArray data) {
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralWriteCommand(JNIEnv *env, jobject thiz,
                                                              jlong adapter_id, jlong instance_id,
                                                              jstring service, jstring characteristic,
                                                              jbyteArray data) {
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralDescriptorRead(JNIEnv *env, jobject thiz,
                                                                jlong adapter_id, jlong instance_id,
                                                                jstring service, jstring characteristic,
                                                                jstring descriptor) {
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralDescriptorWrite(JNIEnv *env, jobject thiz,
                                                                 jlong adapter_id, jlong instance_id,
                                                                 jstring service, jstring characteristic,
                                                                 jstring descriptor, jbyteArray data) {
}