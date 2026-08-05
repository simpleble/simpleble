#include "Characteristic.h"

namespace Org {
namespace SimpleBLE {
namespace Android {

SimpleJNI::GlobalRef<jclass> Characteristic::_cls;
jmethodID Characteristic::_init_method = nullptr;

const SimpleJNI::JNIDescriptor Characteristic::descriptor{
    "org/simpleble/android/Characteristic",
    &_cls,
    {{"<init>", "(Ljava/lang/String;Ljava/util/List;ZZZZZ)V", &_init_method}}};

const SimpleJNI::AutoRegister<Characteristic> Characteristic::registrar{&descriptor};

Characteristic::Characteristic(const SimpleJNI::String<SimpleJNI::LocalRef>& uuid,
                               const SimpleJNI::Java::Util::ArrayList& descriptors, bool can_read,
                               bool can_write_request, bool can_write_command, bool can_notify, bool can_indicate) {
    _obj = SimpleJNI::Object<SimpleJNI::ReleasableLocalRef>::call_new_object(
        _cls.get(), _init_method, uuid.get(), descriptors.get(), static_cast<jboolean>(can_read),
        static_cast<jboolean>(can_write_request), static_cast<jboolean>(can_write_command),
        static_cast<jboolean>(can_notify), static_cast<jboolean>(can_indicate));
}

Characteristic::operator SimpleJNI::Object<SimpleJNI::ReleasableLocalRef, jobject>() const { return _obj; }

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
