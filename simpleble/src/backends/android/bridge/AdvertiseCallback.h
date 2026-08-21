#pragma once

#include <functional>

#include <kvn_safe_callback.hpp>
#include <kvn_safe_map.hpp>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleBLE::Android::Bridge {

class AdvertiseCallback {
  public:
    AdvertiseCallback();
    ~AdvertiseCallback();

    jobject get() const { return _obj.get(); }

    void set_callback_onStartSuccess(std::function<void()> callback);
    void set_callback_onStartFailure(std::function<void(int)> callback);

    static void jni_onStartSuccessCallback(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> thiz);
    static void jni_onStartFailureCallback(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> thiz, int error_code);

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;
    kvn::safe_callback<void()> _on_success;
    kvn::safe_callback<void(int)> _on_failure;

    static kvn::safe_map<SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>, AdvertiseCallback*,
                         SimpleJNI::ObjectComparator<SimpleJNI::GlobalRef, jobject>>
        _instances;
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _constructor;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<AdvertiseCallback> registrar;
};

}  // namespace SimpleBLE::Android::Bridge
