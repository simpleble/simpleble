#include "Service.h"

namespace Org {
namespace SimpleKtBLE {

SimpleJNI::GlobalRef<jclass> Service::_cls;
jmethodID Service::_init_method = nullptr;

const SimpleJNI::JNIDescriptor Service::descriptor{
    "org/simplektble/Service",
    &_cls,
    {{"<init>", "(Ljava/lang/String;[BLjava/util/List;)V", &_init_method}}};

const SimpleJNI::AutoRegister<Service> Service::registrar{&descriptor};

Service::Service(const SimpleJNI::String<SimpleJNI::LocalRef>& uuid,
                 const SimpleJNI::ByteArray<SimpleJNI::LocalRef>& data,
                 const SimpleJNI::Java::Util::ArrayList& characteristics) {
    _obj = SimpleJNI::Object<SimpleJNI::ReleasableLocalRef>::call_new_object(_cls.get(), _init_method, uuid.get(),
                                                                             data.get(), characteristics.get());
}

Service::operator SimpleJNI::Object<SimpleJNI::ReleasableLocalRef, jobject>() const { return _obj; }

}  // namespace SimpleKtBLE
}  // namespace Org
