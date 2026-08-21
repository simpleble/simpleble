#include "UUID.h"

namespace SimpleBLE {
namespace Android {

SimpleJNI::GlobalRef<jclass> UUID::_cls;
jmethodID UUID::_method_toString = nullptr;
jmethodID UUID::_method_fromString = nullptr;

const SimpleJNI::JNIDescriptor UUID::descriptor{"java/util/UUID",  // Java class name
                                                &_cls,             // Pointer to store the jclass
                                                {                  // Methods to preload
                                                 {"toString", "()Ljava/lang/String;", &_method_toString}}};

const SimpleJNI::StaticJNIDescriptor UUID::static_descriptor{
    "java/util/UUID", &_cls, {{"fromString", "(Ljava/lang/String;)Ljava/util/UUID;", &_method_fromString}}};

const SimpleJNI::AutoRegister<UUID> UUID::registrar{&descriptor, &static_descriptor};

UUID::UUID() : _obj() {}

UUID::UUID(SimpleJNI::Object<SimpleJNI::GlobalRef, jobject> obj) : _obj(obj) {}

std::string UUID::toString() {
    if (!_obj) throw std::runtime_error("UUID is not initialized");
    return _obj.call_string_method(_method_toString);
}

UUID UUID::fromString(const std::string& value) {
    SimpleJNI::Env env;
    SimpleJNI::String<SimpleJNI::LocalRef> string(value);
    jobject local = env->CallStaticObjectMethod(_cls.get(), _method_fromString, string.get());
    SimpleJNI::Exception::check(env);
    SimpleJNI::Object<SimpleJNI::LocalRef, jobject> object(SimpleJNI::adopt_local_ref, local);
    return UUID(object.to_global());
}

}  // namespace Android
}  // namespace SimpleBLE
