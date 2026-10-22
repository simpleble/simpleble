#include <jni.h>
#include <string>

#include "fmt/format.h"
#include <simpleble/Advanced.h>
#include <simpleble/Backend.h>
#include <simpleble/Logging.h>
#include <simpleble/SimpleBLE.h>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "core/AdapterWrapper.h"
#include "core/Cache.h"
#include "org/simplejavable/AdapterCallback.h"
#include "org/simplejavable/Characteristic.h"
#include "org/simplejavable/DataCallback.h"
#include "org/simplejavable/Descriptor.h"
#include "org/simplejavable/PeripheralCallback.h"
#include "org/simplejavable/Service.h"
#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"
#include "simplejni/java/lang/Integer.hpp"
#include "simplejni/java/util/ArrayList.hpp"
#include "simplejni/java/util/HashMap.hpp"

using namespace SimpleJNI;

// Raised by advanced APIs whose platform backend is not part of this build.
class UnsupportedOperation : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

static void throw_java_exception(JNIEnv* env, const char* class_name, const char* message) {
    // If Java already has a pending exception, keep it, as it has more details.
    if (env->ExceptionCheck()) return;

    jclass exception_class = env->FindClass(class_name);
    if (exception_class != nullptr) {
        env->ThrowNew(exception_class, message);
        env->DeleteLocalRef(exception_class);
    }
}

template <typename Func>
auto safe_jni_call(JNIEnv* env, Func&& func) {
    using ReturnType = decltype(func());
    try {
        return func();
    } catch (const SimpleBLE::Exception::OperationNotSupported& e) {
        throw_java_exception(env, "java/lang/UnsupportedOperationException", e.what());
    } catch (const UnsupportedOperation& e) {
        throw_java_exception(env, "java/lang/UnsupportedOperationException", e.what());
    } catch (const SimpleBLE::Exception::InvalidArgument& e) {
        throw_java_exception(env, "java/lang/IllegalArgumentException", e.what());
    } catch (const std::exception& e) {
        throw_java_exception(env, "java/lang/RuntimeException", e.what());
    } catch (...) {
        throw_java_exception(env, "java/lang/RuntimeException", "Unknown native error");
    }
    if constexpr (!std::is_void_v<ReturnType>) {
        return ReturnType{};
    }
}

static std::vector<std::string> string_array(JNIEnv* env, jobjectArray array) {
    std::vector<std::string> values;
    if (array == nullptr) return values;

    const jsize count = env->GetArrayLength(array);
    values.reserve(count);
    for (jsize index = 0; index < count; index++) {
        auto value = static_cast<jstring>(env->GetObjectArrayElement(array, index));
        Exception::check(env);
        values.push_back(String<LocalRef>(value).str());
        env->DeleteLocalRef(value);
    }
    return values;
}

static jlongArray peripheral_ids(int64_t adapter_id, const std::vector<SimpleBLE::Peripheral>& peripherals) {
    std::vector<int64_t> ids;
    for (const SimpleBLE::Peripheral& peripheral : peripherals) {
        ids.push_back(Cache::get().addPeripheral(adapter_id, peripheral));
    }
    return LongArray<ReleasableLocalRef>(ids).release();
}


JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    SimpleJNI::Env env;
    try {
        SimpleJNI::Registrar::get().preload(env);
    } catch (const std::exception& e) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_simplejavable_Adapter_nativeIsBluetoothEnabled(JNIEnv* env, jclass clazz) {
    return safe_jni_call(env, [&]() {

        return SimpleBLE::Adapter::bluetooth_enabled();
    });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Adapter_nativeGetAdapters(JNIEnv* env, jclass clazz) {
    return safe_jni_call(env, [&]() {

        std::vector<int64_t> adapter_ids;
        for (const SimpleBLE::Adapter& adapter : SimpleBLE::Adapter::get_adapters()) {
            adapter_ids.push_back(Cache::get().addAdapter(adapter));
        }

        LongArray<ReleasableLocalRef> adapter_ids_array(adapter_ids);
        return adapter_ids_array.release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterRegister(JNIEnv* env, jobject thiz,
                                                                                       jlong adapter_id,
                                                                                       jobject callback) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        adapter_wrapper->setCallback(callback);
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Adapter_nativeAdapterIdentifier(JNIEnv* env, jobject thiz,
                                                                                            jlong adapter_id) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        return String<ReleasableLocalRef>(adapter_wrapper->get().identifier()).release();
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Adapter_nativeAdapterAddress(JNIEnv* env, jobject thiz,
                                                                                         jlong adapter_id) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        return String<ReleasableLocalRef>(adapter_wrapper->get().address()).release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanStart(JNIEnv* env, jobject thiz,
                                                                                        jlong adapter_id) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        adapter_wrapper->get().scan_start();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanStop(JNIEnv* env, jobject thiz,
                                                                                       jlong adapter_id) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        adapter_wrapper->get().scan_stop();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanFor(JNIEnv* env, jobject thiz,
                                                                                      jlong adapter_id, jint timeout) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        adapter_wrapper->get().scan_for(timeout);
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanIsActive(JNIEnv* env,
                                                                                               jobject thiz,
                                                                                               jlong adapter_id) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        return adapter_wrapper->get().scan_is_active();
    });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Adapter_nativeAdapterScanGetResults(JNIEnv* env,
                                                                                                   jobject thiz,
                                                                                                   jlong adapter_id) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        return peripheral_ids(adapter_id, adapter_wrapper->get().scan_get_results());
    });
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_org_simplejavable_Adapter_nativeAdapterGetPairedPeripherals(JNIEnv* env, jobject thiz, jlong adapter_id) {
    return safe_jni_call(env, [&]() {

        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        return peripheral_ids(adapter_id, adapter_wrapper->get().get_paired_peripherals());
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplejavable_Adapter_nativeAdapterCreateLocalPeripheral(JNIEnv* env,
                                                                                                     jobject thiz,
                                                                                                     jlong adapter_id) {
    return safe_jni_call(env, [&]() {
        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        return static_cast<jlong>(Cache::get().addLocalPeripheral(adapter_wrapper->get().create_local_peripheral()));
    });
}

// PERIPHERAL

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralRegister(JNIEnv* env, jobject thiz,
                                                                                             jlong adapter_id,
                                                                                             jlong peripheral_id,
                                                                                             jobject callback) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        peripheral_wrapper->setCallback(callback);
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Peripheral_nativePeripheralIdentifier(JNIEnv* env,
                                                                                                  jobject thiz,
                                                                                                  jlong adapter_id,
                                                                                                  jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return String<ReleasableLocalRef>(peripheral_wrapper->get().identifier()).release();
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Peripheral_nativePeripheralAddress(JNIEnv* env,
                                                                                               jobject thiz,
                                                                                               jlong adapter_id,
                                                                                               jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return String<ReleasableLocalRef>(peripheral_wrapper->get().address()).release();
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplejavable_Peripheral_nativePeripheralAddressType(JNIEnv* env,
                                                                                                jobject thiz,
                                                                                                jlong adapter_id,
                                                                                                jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return peripheral_wrapper->get().address_type();
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplejavable_Peripheral_nativePeripheralRssi(JNIEnv* env, jobject thiz,
                                                                                         jlong adapter_id,
                                                                                         jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return peripheral_wrapper->get().rssi();
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplejavable_Peripheral_nativePeripheralTxPower(JNIEnv* env, jobject thiz,
                                                                                            jlong adapter_id,
                                                                                            jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return peripheral_wrapper->get().tx_power();
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplejavable_Peripheral_nativePeripheralMtu(JNIEnv* env, jobject thiz,
                                                                                        jlong adapter_id,
                                                                                        jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return peripheral_wrapper->get().mtu();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralConnect(JNIEnv* env, jobject thiz,
                                                                                            jlong adapter_id,
                                                                                            jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        peripheral_wrapper->get().connect();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralDisconnect(JNIEnv* env,
                                                                                               jobject thiz,
                                                                                               jlong adapter_id,
                                                                                               jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        peripheral_wrapper->get().disconnect();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralNotify(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jobject callback) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        Org::SimpleJavaBLE::DataCallback data_callback(callback);
        String<LocalRef> service(j_service);
        String<LocalRef> characteristic(j_characteristic);
        peripheral_wrapper->get().notify(service.str(), characteristic.str(),
                                         [data_callback](SimpleBLE::ByteArray payload) {
                                             Org::SimpleJavaBLE::DataCallback data_callback_local(data_callback);
                                             ByteArray<ReleasableLocalRef> j_payload(payload);
                                             data_callback_local.on_data_received(j_payload.release());
                                         });
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralIndicate(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jobject callback) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        Org::SimpleJavaBLE::DataCallback data_callback(callback);
        String<LocalRef> service(j_service);
        String<LocalRef> characteristic(j_characteristic);
        peripheral_wrapper->get().indicate(service.str(), characteristic.str(),
                                           [data_callback](SimpleBLE::ByteArray payload) {
                                               Org::SimpleJavaBLE::DataCallback data_callback_local(data_callback);
                                               ByteArray<ReleasableLocalRef> j_payload(payload);
                                               data_callback_local.on_data_received(j_payload.release());
                                           });
    });
}
extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralUnsubscribe(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        String<LocalRef> service(j_service);
        String<LocalRef> characteristic(j_characteristic);
        peripheral_wrapper->get().unsubscribe(service.str(), characteristic.str());
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Peripheral_nativePeripheralIsConnected(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return peripheral_wrapper->get().is_connected();
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Peripheral_nativePeripheralIsConnectable(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return peripheral_wrapper->get().is_connectable();
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Peripheral_nativePeripheralIsPaired(JNIEnv* env,
                                                                                                 jobject thiz,
                                                                                                 jlong adapter_id,
                                                                                                 jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        return peripheral_wrapper->get().is_paired();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralUnpair(JNIEnv* env, jobject thiz,
                                                                                           jlong adapter_id,
                                                                                           jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        peripheral_wrapper->get().unpair();
    });
}

extern "C" JNIEXPORT jobject JNICALL Java_org_simplejavable_Peripheral_nativePeripheralServices(JNIEnv* env,
                                                                                                jobject thiz,
                                                                                                jlong adapter_id,
                                                                                                jlong peripheral_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);

        SimpleJNI::Java::Util::ArrayList services_list;
        for (auto& service : peripheral_wrapper->get().services()) {
            SimpleJNI::Java::Util::ArrayList characteristics_list;
            for (auto& characteristic : service.characteristics()) {
                SimpleJNI::Java::Util::ArrayList descriptors_list;
                for (auto& descriptor : characteristic.descriptors()) {
                    Org::SimpleJavaBLE::Descriptor descriptor_obj(descriptor.uuid());
                    descriptors_list.add(descriptor_obj);
                }
                Org::SimpleJavaBLE::Characteristic characteristic_obj(
                    characteristic.uuid(), descriptors_list, characteristic.can_read(),
                    characteristic.can_write_request(), characteristic.can_write_command(), characteristic.can_notify(),
                    characteristic.can_indicate());
                characteristics_list.add(characteristic_obj);
            }
            Org::SimpleJavaBLE::Service service_obj(service.uuid(), characteristics_list);
            services_list.add(service_obj);
        }
        return services_list.release();
    });
}

extern "C" JNIEXPORT jobject JNICALL Java_org_simplejavable_Peripheral_nativePeripheralManufacturerData(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong instance_id) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, instance_id);
        auto manufacturer_data = peripheral_wrapper->get().manufacturer_data();

        SimpleJNI::Java::Util::HashMap hash_map;
        for (const auto& [key, value] : manufacturer_data) {
            SimpleJNI::Java::Lang::Integer integer(static_cast<jint>(key));
            ByteArray<LocalRef> bytes(value);
            hash_map.put(integer.get(), bytes.get());
        }
        return hash_map.release();
    });
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_simplejavable_Peripheral_nativePeripheralRead(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        String<LocalRef> service(j_service);
        String<LocalRef> characteristic(j_characteristic);
        return ByteArray<ReleasableLocalRef>(peripheral_wrapper->get().read(service.str(), characteristic.str())).release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralWriteRequest(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jbyteArray j_data) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        String<LocalRef> service(j_service);
        String<LocalRef> characteristic(j_characteristic);
        ByteArray<LocalRef> data(j_data);
        peripheral_wrapper->get().write_request(service.str(), characteristic.str(), data.bytes());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralWriteCommand(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jbyteArray j_data) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        String<LocalRef> service(j_service);
        String<LocalRef> characteristic(j_characteristic);
        ByteArray<LocalRef> data(j_data);
        peripheral_wrapper->get().write_command(service.str(), characteristic.str(), data.bytes());
    });
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_simplejavable_Peripheral_nativePeripheralDescriptorRead(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jstring j_descriptor) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        String<LocalRef> service(j_service);
        String<LocalRef> characteristic(j_characteristic);
        String<LocalRef> descriptor(j_descriptor);
        return ByteArray<ReleasableLocalRef>(
                   peripheral_wrapper->get().read(service.str(), characteristic.str(), descriptor.str()))
            .release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Peripheral_nativePeripheralDescriptorWrite(
    JNIEnv* env, jobject thiz, jlong adapter_id, jlong peripheral_id, jstring j_service, jstring j_characteristic,
    jstring j_descriptor, jbyteArray j_data) {
    return safe_jni_call(env, [&]() {

        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        String<LocalRef> service(j_service);
        String<LocalRef> characteristic(j_characteristic);
        String<LocalRef> descriptor(j_descriptor);
        ByteArray<LocalRef> data(j_data);
        peripheral_wrapper->get().write(service.str(), characteristic.str(), descriptor.str(), data.bytes());
    });
}

// BACKEND

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Backend_nativeGetBackends(JNIEnv* env, jclass clazz) {
    return safe_jni_call(env, [&]() {
        std::vector<int64_t> backend_ids;
        for (const SimpleBLE::Backend& backend : SimpleBLE::Backend::get_backends()) {
            backend_ids.push_back(Cache::get().addBackend(backend));
        }
        return LongArray<ReleasableLocalRef>(backend_ids).release();
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplejavable_Backend_nativeGetNative(JNIEnv* env, jclass clazz) {
    return safe_jni_call(env, [&]() {
        return static_cast<jlong>(Cache::get().addBackend(SimpleBLE::Backend::get_native()));
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplejavable_Backend_nativeGetDongl(JNIEnv* env, jclass clazz) {
    return safe_jni_call(env, [&]() {
        return static_cast<jlong>(Cache::get().addBackend(SimpleBLE::Backend::get_dongl()));
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplejavable_Backend_nativeGetSimulator(JNIEnv* env, jclass clazz) {
    return safe_jni_call(env, [&]() {
        return static_cast<jlong>(Cache::get().addBackend(SimpleBLE::Backend::get_simulator()));
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_Backend_nativeBackendIdentifier(JNIEnv* env, jobject thiz,
                                                                                            jlong backend_id) {
    return safe_jni_call(env, [&]() {
        return String<ReleasableLocalRef>(Cache::get().getBackend(backend_id).identifier()).release();
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Backend_nativeBackendIsBluetoothEnabled(JNIEnv* env,
                                                                                                     jobject thiz,
                                                                                                     jlong backend_id) {
    return safe_jni_call(env, [&]() {
        return static_cast<jboolean>(Cache::get().getBackend(backend_id).bluetooth_enabled());
    });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Backend_nativeBackendGetAdapters(JNIEnv* env,
                                                                                                jobject thiz,
                                                                                                jlong backend_id) {
    return safe_jni_call(env, [&]() {
        std::vector<int64_t> adapter_ids;
        for (const SimpleBLE::Adapter& adapter : Cache::get().getBackend(backend_id).adapters()) {
            adapter_ids.push_back(Cache::get().addAdapter(adapter));
        }
        return LongArray<ReleasableLocalRef>(adapter_ids).release();
    });
}

// CONFIG

static jlong duration_to_nanos(std::chrono::steady_clock::duration duration) {
    return static_cast<jlong>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

static std::chrono::steady_clock::duration nanos_to_duration(jlong nanos) {
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::nanoseconds(nanos));
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeResetAll(JNIEnv* env, jclass clazz) {
    SimpleBLE::Config::Base::reset_all();
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeSimpleBluezReset(JNIEnv* env, jclass clazz) {
    SimpleBLE::Config::SimpleBluez::reset();
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Config_nativeSimpleBluezGetUseSystemBus(JNIEnv* env,
                                                                                                     jclass clazz) {
    return SimpleBLE::Config::SimpleBluez::use_system_bus;
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeSimpleBluezSetUseSystemBus(JNIEnv* env,
                                                                                                 jclass clazz,
                                                                                                 jboolean value) {
    SimpleBLE::Config::SimpleBluez::use_system_bus = value;
}

extern "C" JNIEXPORT jlong JNICALL
Java_org_simplejavable_Config_nativeSimpleBluezGetConnectionTimeout(JNIEnv* env, jclass clazz) {
    return duration_to_nanos(SimpleBLE::Config::SimpleBluez::connection_timeout);
}

extern "C" JNIEXPORT void JNICALL
Java_org_simplejavable_Config_nativeSimpleBluezSetConnectionTimeout(JNIEnv* env, jclass clazz, jlong nanos) {
    SimpleBLE::Config::SimpleBluez::connection_timeout = nanos_to_duration(nanos);
}

extern "C" JNIEXPORT jlong JNICALL
Java_org_simplejavable_Config_nativeSimpleBluezGetDisconnectionTimeout(JNIEnv* env, jclass clazz) {
    return duration_to_nanos(SimpleBLE::Config::SimpleBluez::disconnection_timeout);
}

extern "C" JNIEXPORT void JNICALL
Java_org_simplejavable_Config_nativeSimpleBluezSetDisconnectionTimeout(JNIEnv* env, jclass clazz, jlong nanos) {
    SimpleBLE::Config::SimpleBluez::disconnection_timeout = nanos_to_duration(nanos);
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeWinRTReset(JNIEnv* env, jclass clazz) {
    SimpleBLE::Config::WinRT::reset();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_simplejavable_Config_nativeWinRTGetExperimentalUseOwnMtaApartment(JNIEnv* env, jclass clazz) {
    return SimpleBLE::Config::WinRT::experimental_use_own_mta_apartment;
}

extern "C" JNIEXPORT void JNICALL
Java_org_simplejavable_Config_nativeWinRTSetExperimentalUseOwnMtaApartment(JNIEnv* env, jclass clazz, jboolean value) {
    SimpleBLE::Config::WinRT::experimental_use_own_mta_apartment = value;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_simplejavable_Config_nativeWinRTGetExperimentalReinitializeWinrtApartmentOnMainThread(JNIEnv* env,
                                                                                               jclass clazz) {
    return SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread;
}

extern "C" JNIEXPORT void JNICALL
Java_org_simplejavable_Config_nativeWinRTSetExperimentalReinitializeWinrtApartmentOnMainThread(JNIEnv* env,
                                                                                               jclass clazz,
                                                                                               jboolean value) {
    SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread = value;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Config_nativeWinRTGetUseDeferredDisconnect(JNIEnv* env,
                                                                                                        jclass clazz) {
    return SimpleBLE::Config::WinRT::use_deferred_disconnect;
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeWinRTSetUseDeferredDisconnect(JNIEnv* env,
                                                                                                    jclass clazz,
                                                                                                    jboolean value) {
    SimpleBLE::Config::WinRT::use_deferred_disconnect = value;
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeCoreBluetoothReset(JNIEnv* env, jclass clazz) {
    SimpleBLE::Config::CoreBluetooth::reset();
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeAndroidReset(JNIEnv*, jclass) {
    SimpleBLE::Config::Android::reset();
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Config_nativeAndroidGetNegotiateMtu(JNIEnv*, jclass) {
    return SimpleBLE::Config::Android::negotiate_mtu;
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeAndroidSetNegotiateMtu(JNIEnv*, jclass, jboolean value) {
    SimpleBLE::Config::Android::negotiate_mtu = value;
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeDonglReset(JNIEnv* env, jclass clazz) {
    SimpleBLE::Config::Dongl::reset();
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Config_nativeDonglGetUseDonglBackend(JNIEnv* env,
                                                                                                  jclass clazz) {
    return SimpleBLE::Config::Dongl::use_dongl_backend;
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeDonglSetUseDonglBackend(JNIEnv* env,
                                                                                              jclass clazz,
                                                                                              jboolean value) {
    SimpleBLE::Config::Dongl::use_dongl_backend = value;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Config_nativeDonglGetAutoUpdate(JNIEnv* env,
                                                                                             jclass clazz) {
    return SimpleBLE::Config::Dongl::auto_update;
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeDonglSetAutoUpdate(JNIEnv* env, jclass clazz,
                                                                                         jboolean value) {
    SimpleBLE::Config::Dongl::auto_update = value;
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_Config_nativeDonglGetForceUpdate(JNIEnv* env,
                                                                                              jclass clazz) {
    return SimpleBLE::Config::Dongl::force_update;
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Config_nativeDonglSetForceUpdate(JNIEnv* env, jclass clazz,
                                                                                          jboolean value) {
    SimpleBLE::Config::Dongl::force_update = value;
}

// LOCAL PERIPHERAL

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalPeripheral_nativeRegister(JNIEnv* env, jobject thiz,
                                                                                        jlong peripheral_id,
                                                                                        jobject callback) {
    return safe_jni_call(env, [&]() {
        Cache::get().setLocalPeripheralCallback(peripheral_id, callback);

        // Callbacks look up the Java object by ID, so the native object doesn't hold a reference to itself.
        const int64_t id = peripheral_id;
        auto peripheral = Cache::get().getLocalPeripheral(id);
        peripheral.set_callback_on_client_connected([id](SimpleBLE::BluetoothAddress address) {
            if (auto local_callback = Cache::get().getLocalPeripheralCallback(id)) {
                local_callback->on_client_connected(address);
            }
        });
        peripheral.set_callback_on_client_disconnected([id](SimpleBLE::BluetoothAddress address) {
            if (auto local_callback = Cache::get().getLocalPeripheralCallback(id)) {
                local_callback->on_client_disconnected(address);
            }
        });
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalPeripheral_nativeAddAdvertisedService(
    JNIEnv* env, jobject thiz, jlong peripheral_id, jobjectArray service_uuids) {
    return safe_jni_call(env, [&]() {
        Cache::get().getLocalPeripheral(peripheral_id).add_advertised_service(string_array(env, service_uuids));
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalPeripheral_nativeSetAdvertisedManufacturerData(
    JNIEnv* env, jobject thiz, jlong peripheral_id, jint company_id, jbyteArray j_data) {
    return safe_jni_call(env, [&]() {
        ByteArray<LocalRef> data(j_data);
        Cache::get().getLocalPeripheral(peripheral_id).set_advertised_manufacturer_data(static_cast<uint16_t>(company_id),
                                                                                        data.bytes());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalPeripheral_nativeSetConnectable(JNIEnv* env, jobject thiz,
                                                                                              jlong peripheral_id,
                                                                                              jboolean connectable) {
    return safe_jni_call(env, [&]() { Cache::get().getLocalPeripheral(peripheral_id).set_connectable(connectable); });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplejavable_LocalPeripheral_nativeAddService(JNIEnv* env, jobject thiz,
                                                                                           jlong peripheral_id,
                                                                                           jstring j_uuid) {
    return safe_jni_call(env, [&]() {
        String<LocalRef> uuid(j_uuid);
        auto service = Cache::get().getLocalPeripheral(peripheral_id).add_service(uuid.str());
        return static_cast<jlong>(Cache::get().addLocalService(service));
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalPeripheral_nativeRemoveAllServices(JNIEnv* env,
                                                                                                 jobject thiz,
                                                                                                 jlong peripheral_id) {
    return safe_jni_call(env, [&]() { Cache::get().getLocalPeripheral(peripheral_id).remove_all_services(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalPeripheral_nativeStart(JNIEnv* env, jobject thiz,
                                                                                     jlong peripheral_id) {
    return safe_jni_call(env, [&]() { Cache::get().getLocalPeripheral(peripheral_id).start(); });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalPeripheral_nativeStop(JNIEnv* env, jobject thiz,
                                                                                    jlong peripheral_id) {
    return safe_jni_call(env, [&]() { Cache::get().getLocalPeripheral(peripheral_id).stop(); });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_LocalPeripheral_nativeIsStarted(JNIEnv* env,
                                                                                             jobject thiz,
                                                                                             jlong peripheral_id) {
    return safe_jni_call(env, [&]() {
        return static_cast<jboolean>(Cache::get().getLocalPeripheral(peripheral_id).is_started());
    });
}

extern "C" JNIEXPORT jboolean JNICALL Java_org_simplejavable_LocalPeripheral_nativeIsAdvertising(JNIEnv* env,
                                                                                                 jobject thiz,
                                                                                                 jlong peripheral_id) {
    return safe_jni_call(env, [&]() {
        return static_cast<jboolean>(Cache::get().getLocalPeripheral(peripheral_id).is_advertising());
    });
}

// LOCAL SERVICE

// Bit values of org.simplejavable.LocalCharacteristicCapability.
static const std::pair<SimpleBLE::Local::CharacteristicCapability, jint> capability_bits[] = {
    {SimpleBLE::Local::CharacteristicCapability::READ, 1},
    {SimpleBLE::Local::CharacteristicCapability::WRITE_REQUEST, 2},
    {SimpleBLE::Local::CharacteristicCapability::WRITE_COMMAND, 4},
    {SimpleBLE::Local::CharacteristicCapability::NOTIFY, 8},
    {SimpleBLE::Local::CharacteristicCapability::INDICATE, 16},
};

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_LocalService_nativeUuid(JNIEnv* env, jobject thiz,
                                                                                    jlong service_id) {
    return safe_jni_call(env, [&]() {
        return String<ReleasableLocalRef>(Cache::get().getLocalService(service_id).uuid()).release();
    });
}

extern "C" JNIEXPORT jlong JNICALL Java_org_simplejavable_LocalService_nativeAddCharacteristic(JNIEnv* env,
                                                                                               jobject thiz,
                                                                                               jlong service_id,
                                                                                               jstring j_uuid,
                                                                                               jint capabilities) {
    return safe_jni_call(env, [&]() {
        std::set<SimpleBLE::Local::CharacteristicCapability> native_capabilities;
        for (const auto& [capability, bit] : capability_bits) {
            if ((capabilities & bit) != 0) {
                native_capabilities.insert(capability);
            }
        }

        String<LocalRef> uuid(j_uuid);
        auto characteristic = Cache::get().getLocalService(service_id).add_characteristic(uuid.str(),
                                                                                          native_capabilities);
        return static_cast<jlong>(Cache::get().addLocalCharacteristic(characteristic));
    });
}

// LOCAL CHARACTERISTIC

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalCharacteristic_nativeRegister(JNIEnv* env, jobject thiz,
                                                                                            jlong characteristic_id,
                                                                                            jobject callback) {
    return safe_jni_call(env, [&]() {
        Cache::get().setLocalCharacteristicCallback(characteristic_id, callback);

        // Callbacks look up the Java object by ID, so the native object doesn't hold a reference to itself.
        const int64_t id = characteristic_id;
        auto characteristic = Cache::get().getLocalCharacteristic(id);
        characteristic.set_callback_on_write([id](SimpleBLE::ByteArray value) {
            if (auto local_callback = Cache::get().getLocalCharacteristicCallback(id)) {
                local_callback->on_write(value);
            }
        });
        characteristic.set_callback_on_subscribed([id]() {
            if (auto local_callback = Cache::get().getLocalCharacteristicCallback(id)) {
                local_callback->on_subscribed();
            }
        });
        characteristic.set_callback_on_unsubscribed([id]() {
            if (auto local_callback = Cache::get().getLocalCharacteristicCallback(id)) {
                local_callback->on_unsubscribed();
            }
        });
    });
}

extern "C" JNIEXPORT jstring JNICALL Java_org_simplejavable_LocalCharacteristic_nativeUuid(JNIEnv* env, jobject thiz,
                                                                                           jlong characteristic_id) {
    return safe_jni_call(env, [&]() {
        return String<ReleasableLocalRef>(Cache::get().getLocalCharacteristic(characteristic_id).uuid()).release();
    });
}

extern "C" JNIEXPORT jint JNICALL Java_org_simplejavable_LocalCharacteristic_nativeCapabilities(
    JNIEnv* env, jobject thiz, jlong characteristic_id) {
    return safe_jni_call(env, [&]() {
        const auto capabilities = Cache::get().getLocalCharacteristic(characteristic_id).capabilities();
        jint mask = 0;
        for (const auto& [capability, bit] : capability_bits) {
            if (capabilities.count(capability) > 0) {
                mask |= bit;
            }
        }
        return mask;
    });
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_org_simplejavable_LocalCharacteristic_nativeValue(
    JNIEnv* env, jobject thiz, jlong characteristic_id) {
    return safe_jni_call(env, [&]() {
        return ByteArray<ReleasableLocalRef>(Cache::get().getLocalCharacteristic(characteristic_id).value()).release();
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalCharacteristic_nativeSetValue(JNIEnv* env, jobject thiz,
                                                                                            jlong characteristic_id,
                                                                                            jbyteArray j_value) {
    return safe_jni_call(env, [&]() {
        ByteArray<LocalRef> value(j_value);
        Cache::get().getLocalCharacteristic(characteristic_id).set_value(value.bytes());
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_LocalCharacteristic_nativeSetReadHandler(
    JNIEnv* env, jobject thiz, jlong characteristic_id, jboolean enabled) {
    return safe_jni_call(env, [&]() {
        auto characteristic = Cache::get().getLocalCharacteristic(characteristic_id);
        if (!enabled) {
            characteristic.set_callback_on_read(nullptr);
            return;
        }

        const int64_t id = characteristic_id;
        characteristic.set_callback_on_read([id]() {
            // If the handler throws or returns null, serve the stored value instead.
            if (auto local_callback = Cache::get().getLocalCharacteristicCallback(id)) {
                if (auto value = local_callback->on_read()) return *value;
            }
            return Cache::get().getLocalCharacteristic(id).value();
        });
    });
}

// ADVANCED

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Advanced_nativeDonglSetPasskeyRequestCallback(
    JNIEnv* env, jclass clazz, jlong adapter_id, jlong peripheral_id, jboolean enabled) {
    return safe_jni_call(env, [&]() {
        Cache::get().getPeripheral(adapter_id, peripheral_id)->setPasskeyRequestCallback(enabled);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Advanced_nativeDonglSetPasskeyDisplayCallback(
    JNIEnv* env, jclass clazz, jlong adapter_id, jlong peripheral_id, jboolean enabled) {
    return safe_jni_call(env, [&]() {
        Cache::get().getPeripheral(adapter_id, peripheral_id)->setPasskeyDisplayCallback(enabled);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Advanced_nativeDonglSetNumericComparisonCallback(
    JNIEnv* env, jclass clazz, jlong adapter_id, jlong peripheral_id, jboolean enabled) {
    return safe_jni_call(env, [&]() {
        Cache::get().getPeripheral(adapter_id, peripheral_id)->setNumericComparisonCallback(enabled);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Advanced_nativeDonglSecureConnection(
    JNIEnv* env, jclass clazz, jlong adapter_id, jlong peripheral_id, jboolean mitm) {
    return safe_jni_call(env, [&]() {
        auto peripheral_wrapper = Cache::get().getPeripheral(adapter_id, peripheral_id);
        SimpleBLE::Advanced::Dongl::secure_connection(peripheral_wrapper->get(), mitm);
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Advanced_nativeMacOSSetAdvertisementLocalName(
    JNIEnv* env, jclass clazz, jlong local_peripheral_id, jstring j_local_name) {
    return safe_jni_call(env, [&]() {
#if SIMPLEBLE_BACKEND_MACOS
        auto peripheral = Cache::get().getLocalPeripheral(local_peripheral_id);
        std::optional<std::string> local_name;
        if (j_local_name != nullptr) {
            local_name = String<LocalRef>(j_local_name).str();
        }
        SimpleBLE::Advanced::MacOS::set_advertisement_local_name(peripheral, local_name);
#else
        throw UnsupportedOperation("Advanced.MacOS requires the macOS backend.");
#endif
    });
}

extern "C" JNIEXPORT jlongArray JNICALL Java_org_simplejavable_Advanced_nativeMacOSRetrieveCachedPeripherals(
    JNIEnv* env, jclass clazz, jlong adapter_id, jobjectArray j_identifiers) {
    return safe_jni_call(env, [&]() -> jlongArray {
#if SIMPLEBLE_BACKEND_MACOS
        auto adapter_wrapper = Cache::get().getAdapter(adapter_id);
        auto peripherals = SimpleBLE::Advanced::MacOS::retrieve_cached_peripherals(adapter_wrapper->get(),
                                                                                   string_array(env, j_identifiers));
        return peripheral_ids(adapter_id, peripherals);
#else
        throw UnsupportedOperation("Advanced.MacOS requires the macOS backend.");
#endif
    });
}

extern "C" JNIEXPORT void JNICALL Java_org_simplejavable_Advanced_nativeLinuxSetAdvertisementLocalName(
    JNIEnv* env, jclass clazz, jlong local_peripheral_id, jstring j_local_name) {
    return safe_jni_call(env, [&]() {
#if SIMPLEBLE_BACKEND_LINUX
        auto peripheral = Cache::get().getLocalPeripheral(local_peripheral_id);
        std::optional<std::string> local_name;
        if (j_local_name != nullptr) {
            local_name = String<LocalRef>(j_local_name).str();
        }
        SimpleBLE::Advanced::Linux::set_advertisement_local_name(peripheral, local_name);
#else
        throw UnsupportedOperation("Advanced.Linux requires the Linux backend.");
#endif
    });
}
