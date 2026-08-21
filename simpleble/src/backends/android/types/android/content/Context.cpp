#include "Context.h"

#include <stdexcept>

namespace SimpleBLE::Android {

SimpleJNI::GlobalRef<jclass> Context::_cls;
jmethodID Context::_get_system_service = nullptr;

const SimpleJNI::JNIDescriptor Context::descriptor{
    "android/content/Context",
    &_cls,
    {{"getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", &_get_system_service}}};
const SimpleJNI::AutoRegister<Context> Context::registrar{&descriptor};

SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> Context::getSystemService(const std::string& name) const {
    if (!_obj) throw std::runtime_error("Android Context is not initialized");
    SimpleJNI::String<SimpleJNI::LocalRef> service_name(name);
    return _obj.call_object_method(_get_system_service, service_name.get()).to_global();
}

}  // namespace SimpleBLE::Android
