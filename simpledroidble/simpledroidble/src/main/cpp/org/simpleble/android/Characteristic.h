#pragma once

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"
#include "simplejni/java/util/ArrayList.hpp"

namespace Org {
namespace SimpleBLE {
namespace Android {

class Characteristic {
  public:
    Characteristic(const SimpleJNI::String<SimpleJNI::LocalRef>& uuid,
                   const SimpleJNI::Java::Util::ArrayList& descriptors, bool can_read, bool can_write_request,
                   bool can_write_command, bool can_notify, bool can_indicate);

    operator SimpleJNI::Object<SimpleJNI::ReleasableLocalRef, jobject>() const;

  private:
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _init_method;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<Characteristic> registrar;

    SimpleJNI::Object<SimpleJNI::ReleasableLocalRef> _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
