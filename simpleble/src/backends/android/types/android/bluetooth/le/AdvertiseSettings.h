#pragma once

#include <utility>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleBLE::Android {

class AdvertiseSettings {
  public:
    AdvertiseSettings() = default;
    explicit AdvertiseSettings(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> object) : _obj(std::move(object)) {}
    jobject get() const { return _obj.get(); }

    class Builder {
      public:
        Builder();
        Builder& setConnectable(bool connectable);
        Builder& setAdvertiseMode(int mode);
        AdvertiseSettings build();

      private:
        SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;
        static SimpleJNI::GlobalRef<jclass> _cls;
        static jmethodID _constructor;
        static jmethodID _set_connectable;
        static jmethodID _set_advertise_mode;
        static jmethodID _build;
        static const SimpleJNI::JNIDescriptor descriptor;
        static const SimpleJNI::AutoRegister<Builder> registrar;
    };

    static constexpr int ADVERTISE_MODE_BALANCED = 1;

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;
};

}  // namespace SimpleBLE::Android
