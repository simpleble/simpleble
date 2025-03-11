#include "List.h"

namespace Java::Util {

template <template <typename> class RefType>
SimpleJNI::GlobalRef<jclass> List<RefType>::_cls;

template <template <typename> class RefType>
jmethodID List<RefType>::_method_iterator = nullptr;

template <template <typename> class RefType>
jmethodID List<RefType>::_method_add = nullptr;

template <template <typename> class RefType>
jmethodID List<RefType>::_method_size = nullptr;

template <template <typename> class RefType>
jmethodID List<RefType>::_method_get = nullptr;

template <template <typename> class RefType>
const SimpleJNI::JNIDescriptor List<RefType>::descriptor{
    "java/util/List",    // Java interface name
    &_cls,               // Where to store the jclass
    {                    // Methods to preload
        {"iterator", "()Ljava/util/Iterator;", &_method_iterator},
        {"add", "(Ljava/lang/Object;)Z", &_method_add},
        {"size", "()I", &_method_size},
        {"get", "(I)Ljava/lang/Object;", &_method_get}
    }
};

template <template <typename> class RefType>
const SimpleJNI::AutoRegister<List<RefType>> List<RefType>::registrar{&descriptor};

template <template <typename> class RefType>
List<RefType>::List() : _obj() {}

template <template <typename> class RefType>
List<RefType>::List(jobject obj) : _obj(obj) {
    if (!_cls.get()) {
        throw std::runtime_error("List JNI resources not preloaded");
    }
}

template <template <typename> class RefType>
template <template <typename> class OtherRefType>
List<RefType>::List(const SimpleJNI::Object<OtherRefType, jobject>& obj) : _obj(obj.get()) {}

template <template <typename> class RefType>
List<SimpleJNI::LocalRef> List<RefType>::to_local() const {
    if (!*this) return List<SimpleJNI::LocalRef>();
    return List<SimpleJNI::LocalRef>(_obj.get());
}

template <template <typename> class RefType>
List<SimpleJNI::GlobalRef> List<RefType>::to_global() const {
    if (!*this) return List<SimpleJNI::GlobalRef>();
    return List<SimpleJNI::GlobalRef>(_obj.get());
}

template <template <typename> class RefType>
jobject List<RefType>::get() const {
    return _obj.get();
}

template <template <typename> class RefType>
List<RefType>::operator bool() const {
    return _obj.get() != nullptr;
}

template <template <typename> class RefType>
Iterator<SimpleJNI::LocalRef> List<RefType>::iterator() const {
    if (!*this) {
        throw std::runtime_error("List is not initialized");
    }
    JNIEnv* env = SimpleJNI::VM::env();
    jobject iter = env->CallObjectMethod(_obj.get(), _method_iterator);
    return Iterator<SimpleJNI::LocalRef>(iter);
}

template <template <typename> class RefType>
void List<RefType>::add(const SimpleJNI::Object<RefType, jobject>& element) {
    if (!*this) {
        throw std::runtime_error("List is not initialized");
    }
    JNIEnv* env = SimpleJNI::VM::env();
    env->CallBooleanMethod(_obj.get(), _method_add, element.get());
}

template <template <typename> class RefType>
size_t List<RefType>::size() const {
    if (!*this) {
        throw std::runtime_error("List is not initialized");
    }
    JNIEnv* env = SimpleJNI::VM::env();
    return static_cast<size_t>(env->CallIntMethod(_obj.get(), _method_size));
}

template <template <typename> class RefType>
SimpleJNI::Object<SimpleJNI::LocalRef> List<RefType>::get(size_t index) const {
    if (!*this) {
        throw std::runtime_error("List is not initialized");
    }
    JNIEnv* env = SimpleJNI::VM::env();
    jobject result = env->CallObjectMethod(_obj.get(), _method_get,
                                         static_cast<jint>(index));
    return SimpleJNI::Object<SimpleJNI::LocalRef>(result);
}

// Explicit template instantiations
template class List<SimpleJNI::LocalRef>;
template class List<SimpleJNI::GlobalRef>;
template class List<SimpleJNI::WeakRef>;

}  // namespace Java::Util