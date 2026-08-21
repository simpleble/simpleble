#include "AdvertiseCallback.h"

#include "CommonUtils.h"

namespace SimpleBLE::Android::Bridge {

SimpleJNI::GlobalRef<jclass> AdvertiseCallback::_cls;
jmethodID AdvertiseCallback::_constructor = nullptr;

const SimpleJNI::JNIDescriptor AdvertiseCallback::descriptor{
    "org/simpleble/android/bridge/AdvertiseCallback", &_cls, {{"<init>", "()V", &_constructor}}};
const SimpleJNI::AutoRegister<AdvertiseCallback> AdvertiseCallback::registrar{&descriptor};

kvn::safe_map<SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>, AdvertiseCallback*,
              SimpleJNI::ObjectComparator<SimpleJNI::GlobalRef, jobject>>
    AdvertiseCallback::_instances;

AdvertiseCallback::AdvertiseCallback() {
    auto object = SimpleJNI::Object<SimpleJNI::LocalRef, jobject>::call_new_object(_cls.get(), _constructor);
    _obj = object.to_global();
    _instances.insert(_obj, this);
}

AdvertiseCallback::~AdvertiseCallback() {
    _on_success.unload();
    _on_failure.unload();
    _instances.erase(_obj);
}

void AdvertiseCallback::set_callback_onStartSuccess(std::function<void()> callback) {
    if (callback) {
        _on_success.load(std::move(callback));
    } else {
        _on_success.unload();
    }
}

void AdvertiseCallback::set_callback_onStartFailure(std::function<void(int)> callback) {
    if (callback) {
        _on_failure.load(std::move(callback));
    } else {
        _on_failure.unload();
    }
}

void AdvertiseCallback::jni_onStartSuccessCallback(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> thiz) {
    if (auto instance = _instances.get(thiz)) {
        SAFE_CALLBACK_CALL(instance.value()->_on_success);
    }
}

void AdvertiseCallback::jni_onStartFailureCallback(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> thiz,
                                                   int error_code) {
    if (auto instance = _instances.get(thiz)) {
        SAFE_CALLBACK_CALL(instance.value()->_on_failure, error_code);
    }
}

}  // namespace SimpleBLE::Android::Bridge

extern "C" {

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_AdvertiseCallback_onStartSuccessCallback(JNIEnv*,
                                                                                                  jobject thiz) {
    SimpleBLE::Android::Bridge::AdvertiseCallback::jni_onStartSuccessCallback(
        SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(thiz));
}

JNIEXPORT void JNICALL Java_org_simpleble_android_bridge_AdvertiseCallback_onStartFailureCallback(JNIEnv*, jobject thiz,
                                                                                                  jint error_code) {
    SimpleBLE::Android::Bridge::AdvertiseCallback::jni_onStartFailureCallback(
        SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>(thiz), error_code);
}
}
