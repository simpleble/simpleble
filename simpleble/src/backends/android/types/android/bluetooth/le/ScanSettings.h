#pragma once

#include <simplejni/Common.hpp>
#include <simplejni/Registry.hpp>

namespace SimpleBLE {
namespace Android {

class ScanSettings : public SimpleJNI::Object<SimpleJNI::GlobalRef> {
  public:
    using SimpleJNI::Object<SimpleJNI::GlobalRef>::Object;

    class Builder : public SimpleJNI::Object<SimpleJNI::GlobalRef> {
      public:
        using SimpleJNI::Object<SimpleJNI::GlobalRef>::Object;
        Builder();
        Builder& setLegacy(bool legacy);
        ScanSettings build();

      private:
        friend class ScanSettings;
        static SimpleJNI::GlobalRef<jclass> _cls;
        static jmethodID _constructor;
        static jmethodID _method_setLegacy;
        static jmethodID _method_build;
        static const SimpleJNI::JNIDescriptor descriptor;
        static const SimpleJNI::AutoRegister<Builder> registrar;
    };

  private:
    friend class Builder;
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _constructor;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<ScanSettings> registrar;
};

}  // namespace Android
}  // namespace SimpleBLE
