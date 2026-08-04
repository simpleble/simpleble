#pragma once

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleBLE {
namespace Android {

class ScanSettings {
  public:
    ScanSettings(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> obj);

    jobject get() const { return _obj.get(); }

    static constexpr int PHY_LE_ALL_SUPPORTED = 0xff;

    class Builder {
      public:
        Builder();

        Builder& setLegacy(bool legacy);
        Builder& setPhy(int phy);
        ScanSettings build();

      private:
        SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;

        static SimpleJNI::GlobalRef<jclass> _cls;
        static jmethodID _constructor;
        static jmethodID _method_setLegacy;
        static jmethodID _method_setPhy;
        static jmethodID _method_build;

        static const SimpleJNI::JNIDescriptor descriptor;
        static const SimpleJNI::AutoRegister<Builder> registrar;
    };

  private:
    SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
