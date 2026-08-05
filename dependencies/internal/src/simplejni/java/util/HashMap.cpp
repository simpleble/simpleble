#include "simplejni/java/util/HashMap.hpp"

namespace SimpleJNI::Java::Util {

jmethodID HashMap::_constructor = nullptr;
jmethodID HashMap::_put = nullptr;

const JNIDescriptor HashMap::_descriptor{
    "java/util/HashMap",
    nullptr,
    {{"<init>", "()V", &_constructor},
     {"put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", &_put}},
};

const AutoRegister<HashMap> HashMap::_registrar{&_descriptor};

HashMap::HashMap() {
    Env env;
    LocalRef<jclass> cls(adopt_local_ref, env->FindClass("java/util/HashMap"));
    Exception::check(env);
    _object = Object<ReleasableLocalRef>::call_new_object(cls.get(), _constructor);
}

void HashMap::put(jobject key, jobject value) { _object.call_object_method(_put, key, value); }

}  // namespace SimpleJNI::Java::Util
