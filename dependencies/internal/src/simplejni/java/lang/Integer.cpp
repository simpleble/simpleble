#include "simplejni/java/lang/Integer.hpp"

namespace SimpleJNI::Java::Lang {

jmethodID Integer::_constructor = nullptr;

const JNIDescriptor Integer::_descriptor{
    "java/lang/Integer",
    nullptr,
    {{"<init>", "(I)V", &_constructor}},
};

const AutoRegister<Integer> Integer::_registrar{&_descriptor};

Integer::Integer(jint value) {
    Env env;
    LocalRef<jclass> cls(adopt_local_ref, env->FindClass("java/lang/Integer"));
    Exception::check(env);
    _object = Object<LocalRef>::call_new_object(cls.get(), _constructor, value);
}

}  // namespace SimpleJNI::Java::Lang
