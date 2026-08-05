#pragma once

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleBLE {
namespace Android {

class Descriptor {
  public:
    explicit Descriptor(const SimpleJNI::String<SimpleJNI::LocalRef>& uuid);

    operator SimpleJNI::Object<SimpleJNI::ReleasableLocalRef, jobject>() const;

  private:
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _init_method;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<Descriptor> registrar;

    SimpleJNI::Object<SimpleJNI::ReleasableLocalRef> _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
