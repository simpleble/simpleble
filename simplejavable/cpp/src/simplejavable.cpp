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

#include "core/Cache.h"
#include "core/AdapterWrapper.h"
#include "jni/Common.hpp"
#include "jni/Registry.hpp"
#include "org/simplejavable/AdapterCallback.h"

using namespace SimpleJNI;

static JavaVM *jvm;


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm;

    SimpleJNI::Env env;

    try {
        SimpleJNI::Registrar::get().preload(env);
    } catch (const std::exception& e) {
        return JNI_ERR;
    }

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

    LongArray<ReleasableLocalRef> adapter_hashes_array(adapter_hashes);
    return adapter_hashes_array.release();
}

extern "C" JNIEXPORT
void JNICALL Java_org_simplejavable_Adapter_nativeAdapterRegister(JNIEnv *env, jobject thiz, jlong adapter_id, jobject callback) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    Org::SimpleJavaBLE::AdapterCallback adapter_callback(callback);
    adapter_wrapper->setCallback(adapter_callback);
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Adapter_nativeAdapterIdentifier(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    return String<ReleasableLocalRef>(adapter_wrapper->get().identifier()).release();
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Adapter_nativeAdapterAddress(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    return String<ReleasableLocalRef>(adapter_wrapper->get().address()).release();
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanStart(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    adapter_wrapper->get().scan_start();
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanStop(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    adapter_wrapper->get().scan_stop();
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanFor(JNIEnv *env, jobject thiz, jlong adapter_id, jint timeout) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    adapter_wrapper->get().scan_for(timeout);
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanIsActive(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    return adapter_wrapper->get().scan_is_active();
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanGetResults(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    std::vector<SimpleBLE::Peripheral> peripherals = adapter_wrapper->get().scan_get_results();

    std::vector<int64_t> peripheral_hashes;
    for (SimpleBLE::Peripheral &peripheral: peripherals) {
        PeripheralWrapper peripheral_wrapper(peripheral);
        peripheral_hashes.push_back(peripheral_wrapper.getHash());
        Cache::get().addPeripheral(adapter_wrapper->getHash(), peripheral_wrapper.getHash(), peripheral_wrapper);
    }

    LongArray<ReleasableLocalRef> peripheral_hashes_array(peripheral_hashes);
    return peripheral_hashes_array.release();
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Adapter_nativeAdapterGetPairedPeripherals(JNIEnv *env, jobject thiz, jlong adapter_id) {
    AdapterWrapper* adapter_wrapper = Cache::get().getAdapter(adapter_id);
    std::vector<SimpleBLE::Peripheral> peripherals = adapter_wrapper->get().paired_peripherals();

    std::vector<int64_t> peripheral_hashes;
    for (SimpleBLE::Peripheral &peripheral: peripherals) {
        PeripheralWrapper peripheral_wrapper(peripheral);
        peripheral_hashes.push_back(peripheral_wrapper.getHash());
        Cache::get().addPeripheral(adapter_wrapper->getHash(), peripheral_wrapper.getHash(), peripheral_wrapper);
    }

    LongArray<ReleasableLocalRef> peripheral_hashes_array(peripheral_hashes);
    return peripheral_hashes_array.release();
}

// PERIPHERAL

extern "C"
JNIEXPORT void JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralRegister(JNIEnv *env, jobject thiz,
                                                          jlong adapter_id, jlong peripheral_id, jobject callback) {
    PeripheralWrapper* peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
    Org::SimpleJavaBLE::PeripheralCallback peripheral_callback(callback);
    peripheral_wrapper->setCallback(peripheral_callback);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralIdentifier(JNIEnv *env, jobject thiz,
                                                            jlong adapter_id, jlong peripheral_id) {
    PeripheralWrapper* peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
    return String<ReleasableLocalRef>(peripheral_wrapper->get().identifier()).release();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_org_simplejavable_Peripheral_nativePeripheralAddress(JNIEnv *env, jobject thiz,
                                                         jlong adapter_id, jlong peripheral_id) {
    PeripheralWrapper* peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
    return String<ReleasableLocalRef>(peripheral_wrapper->get().address()).release();
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