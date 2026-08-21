#include "GattServerCallback.h"

#include "CommonUtils.h"

namespace SimpleBLE::Android::Bridge {

SimpleJNI::GlobalRef<jclass> GattServerCallback::_cls;
jmethodID GattServerCallback::_constructor = nullptr;

const SimpleJNI::JNIDescriptor GattServerCallback::descriptor{
    "org/simpleble/android/bridge/GattServerCallback", &_cls, {{"<init>", "()V", &_constructor}}};
const SimpleJNI::AutoRegister<GattServerCallback> GattServerCallback::registrar{&descriptor};

kvn::safe_map<SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>, GattServerCallback*,
              SimpleJNI::ObjectComparator<SimpleJNI::GlobalRef, jobject>>
    GattServerCallback::_instances;

GattServerCallback::GattServerCallback() {
    auto object = SimpleJNI::Object<SimpleJNI::LocalRef, jobject>::call_new_object(_cls.get(), _constructor);
    _obj = object.to_global();
    _instances.insert(_obj, this);
}

GattServerCallback::~GattServerCallback() {
    on_connection_state_change.unload();
    on_service_added.unload();
    on_characteristic_read.unload();
    on_characteristic_write.unload();
    on_descriptor_read.unload();
    on_descriptor_write.unload();
    on_execute_write.unload();
    on_notification_sent.unload();
    _instances.erase(_obj);
}

void GattServerCallback::set_callback_onConnectionStateChange(std::function<void(BluetoothDevice, int, int)> callback) {
    if (callback) {
        on_connection_state_change.load(std::move(callback));
    } else {
        on_connection_state_change.unload();
    }
}

void GattServerCallback::set_callback_onServiceAdded(std::function<void(int, BluetoothGattService)> callback) {
    if (callback) {
        on_service_added.load(std::move(callback));
    } else {
        on_service_added.unload();
    }
}

void GattServerCallback::set_callback_onCharacteristicReadRequest(
    std::function<void(BluetoothDevice, int, int, BluetoothGattCharacteristic)> callback) {
    if (callback) {
        on_characteristic_read.load(std::move(callback));
    } else {
        on_characteristic_read.unload();
    }
}

void GattServerCallback::set_callback_onCharacteristicWriteRequest(
    std::function<void(BluetoothDevice, int, BluetoothGattCharacteristic, bool, bool, int, std::vector<uint8_t>)>
        callback) {
    if (callback) {
        on_characteristic_write.load(std::move(callback));
    } else {
        on_characteristic_write.unload();
    }
}

void GattServerCallback::set_callback_onDescriptorReadRequest(
    std::function<void(BluetoothDevice, int, int, BluetoothGattDescriptor)> callback) {
    if (callback) {
        on_descriptor_read.load(std::move(callback));
    } else {
        on_descriptor_read.unload();
    }
}

void GattServerCallback::set_callback_onDescriptorWriteRequest(
    std::function<void(BluetoothDevice, int, BluetoothGattDescriptor, bool, bool, int, std::vector<uint8_t>)>
        callback) {
    if (callback) {
        on_descriptor_write.load(std::move(callback));
    } else {
        on_descriptor_write.unload();
    }
}

void GattServerCallback::set_callback_onExecuteWrite(std::function<void(BluetoothDevice, int, bool)> callback) {
    if (callback) {
        on_execute_write.load(std::move(callback));
    } else {
        on_execute_write.unload();
    }
}

void GattServerCallback::set_callback_onNotificationSent(std::function<void(BluetoothDevice, int)> callback) {
    if (callback) {
        on_notification_sent.load(std::move(callback));
    } else {
        on_notification_sent.unload();
    }
}

GattServerCallback* GattServerCallback::find(jobject thiz) {
    auto instance = _instances.get(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(thiz));
    return instance ? instance.value() : nullptr;
}

}  // namespace SimpleBLE::Android::Bridge

namespace {

using Callback = SimpleBLE::Android::Bridge::GattServerCallback;

std::vector<uint8_t> bytes_from(JNIEnv* env, jbyteArray value) {
    if (value == nullptr) return {};
    const jsize size = env->GetArrayLength(value);
    std::vector<uint8_t> result(static_cast<size_t>(size));
    env->GetByteArrayRegion(value, 0, size, reinterpret_cast<jbyte*>(result.data()));
    return result;
}

}  // namespace

extern "C" {

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_GattServerCallback_onConnectionStateChangeCallback(
    JNIEnv*, jobject thiz, jobject device, jint status, jint new_state) {
    if (auto callback = Callback::find(thiz)) {
        SAFE_CALLBACK_CALL(
            callback->on_connection_state_change,
            SimpleBLE::Android::BluetoothDevice(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(device)), status,
            new_state);
    }
}

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_GattServerCallback_onServiceAddedCallback(JNIEnv*,
                                                                                                   jobject thiz,
                                                                                                   jint status,
                                                                                                   jobject service) {
    if (auto callback = Callback::find(thiz)) {
        SAFE_CALLBACK_CALL(
            callback->on_service_added, status,
            SimpleBLE::Android::BluetoothGattService(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(service)));
    }
}

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_GattServerCallback_onCharacteristicReadRequestCallback(
    JNIEnv*, jobject thiz, jobject device, jint request_id, jint offset, jobject characteristic) {
    if (auto callback = Callback::find(thiz)) {
        SAFE_CALLBACK_CALL(
            callback->on_characteristic_read,
            SimpleBLE::Android::BluetoothDevice(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(device)), request_id,
            offset,
            SimpleBLE::Android::BluetoothGattCharacteristic(
                SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(characteristic)));
    }
}

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_GattServerCallback_onCharacteristicWriteRequestCallback(
    JNIEnv* env, jobject thiz, jobject device, jint request_id, jobject characteristic, jboolean prepared_write,
    jboolean response_needed, jint offset, jbyteArray value) {
    if (auto callback = Callback::find(thiz)) {
        SAFE_CALLBACK_CALL(
            callback->on_characteristic_write,
            SimpleBLE::Android::BluetoothDevice(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(device)), request_id,
            SimpleBLE::Android::BluetoothGattCharacteristic(
                SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(characteristic)),
            prepared_write, response_needed, offset, bytes_from(env, value));
    }
}

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_GattServerCallback_onDescriptorReadRequestCallback(
    JNIEnv*, jobject thiz, jobject device, jint request_id, jint offset, jobject descriptor) {
    if (auto callback = Callback::find(thiz)) {
        SAFE_CALLBACK_CALL(
            callback->on_descriptor_read,
            SimpleBLE::Android::BluetoothDevice(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(device)), request_id,
            offset,
            SimpleBLE::Android::BluetoothGattDescriptor(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(descriptor)));
    }
}

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_GattServerCallback_onDescriptorWriteRequestCallback(
    JNIEnv* env, jobject thiz, jobject device, jint request_id, jobject descriptor, jboolean prepared_write,
    jboolean response_needed, jint offset, jbyteArray value) {
    if (auto callback = Callback::find(thiz)) {
        SAFE_CALLBACK_CALL(
            callback->on_descriptor_write,
            SimpleBLE::Android::BluetoothDevice(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(device)), request_id,
            SimpleBLE::Android::BluetoothGattDescriptor(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(descriptor)),
            prepared_write, response_needed, offset, bytes_from(env, value));
    }
}

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_GattServerCallback_onExecuteWriteCallback(
    JNIEnv*, jobject thiz, jobject device, jint request_id, jboolean execute) {
    if (auto callback = Callback::find(thiz)) {
        SAFE_CALLBACK_CALL(
            callback->on_execute_write,
            SimpleBLE::Android::BluetoothDevice(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(device)), request_id,
            execute);
    }
}

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_GattServerCallback_onNotificationSentCallback(JNIEnv*,
                                                                                                       jobject thiz,
                                                                                                       jobject device,
                                                                                                       jint status) {
    if (auto callback = Callback::find(thiz)) {
        SAFE_CALLBACK_CALL(
            callback->on_notification_sent,
            SimpleBLE::Android::BluetoothDevice(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(device)), status);
    }
}
}
