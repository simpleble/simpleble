#pragma once

#include <utility>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"
#include "types/android/os/ParcelUUID.h"

namespace SimpleBLE::Android {

class AdvertiseData {
  public:
    AdvertiseData() = default;
    explicit AdvertiseData(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> object) : _obj(std::move(object)) {}
    jobject get() const { return _obj.get(); }

    class Builder {
      public:
        Builder();
        Builder& addServiceUuid(const ParcelUUID& uuid);
        Builder& setIncludeDeviceName(bool include);
        AdvertiseData build();

      private:
        SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;
        static SimpleJNI::GlobalRef<jclass> _cls;
        static jmethodID _constructor;
        static jmethodID _add_service_uuid;
        static jmethodID _set_include_device_name;
        static jmethodID _build;
        static const SimpleJNI::JNIDescriptor descriptor;
        static const SimpleJNI::AutoRegister<Builder> registrar;
    };

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;
};

}  // namespace SimpleBLE::Android
