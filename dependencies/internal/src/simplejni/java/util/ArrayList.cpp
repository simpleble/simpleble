#include "simplejni/java/util/ArrayList.hpp"

namespace SimpleJNI::Java::Util {

jmethodID ArrayList::_constructor = nullptr;
jmethodID ArrayList::_add = nullptr;

const JNIDescriptor ArrayList::_descriptor{
    "java/util/ArrayList",
    nullptr,
    {{"<init>", "()V", &_constructor}, {"add", "(Ljava/lang/Object;)Z", &_add}},
};

const AutoRegister<ArrayList> ArrayList::_registrar{&_descriptor};

ArrayList::ArrayList() {
    Env env;
    LocalRef<jclass> cls(adopt_local_ref, env->FindClass("java/util/ArrayList"));
    Exception::check(env);
    _object = Object<ReleasableLocalRef>::call_new_object(cls.get(), _constructor);
}

void ArrayList::add(jobject value) { _object.call_boolean_method(_add, value); }

}  // namespace SimpleJNI::Java::Util
