#include "Descriptor.h"

namespace Org {
namespace SimpleBLE {
namespace Android {

SimpleJNI::GlobalRef<jclass> Descriptor::_cls;
jmethodID Descriptor::_init_method = nullptr;

const SimpleJNI::JNIDescriptor Descriptor::descriptor{
    "org/simpleble/android/Descriptor", &_cls, {{"<init>", "(Ljava/lang/String;)V", &_init_method}}};

const SimpleJNI::AutoRegister<Descriptor> Descriptor::registrar{&descriptor};

Descriptor::Descriptor(const SimpleJNI::String<SimpleJNI::LocalRef>& uuid) {
    _obj = SimpleJNI::Object<SimpleJNI::ReleasableLocalRef>::call_new_object(_cls.get(), _init_method, uuid.get());
}

Descriptor::operator SimpleJNI::Object<SimpleJNI::ReleasableLocalRef, jobject>() const { return _obj; }

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
