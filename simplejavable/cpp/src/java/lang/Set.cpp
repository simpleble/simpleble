#include "Set.h"

namespace Java::Util {

template <template <typename> class RefType>
SimpleJNI::GlobalRef<jclass> Set<RefType>::_cls;

template <template <typename> class RefType>
jmethodID Set<RefType>::_method_iterator = nullptr;

template <template <typename> class RefType>
jmethodID Set<RefType>::_method_add = nullptr;

template <template <typename> class RefType>
jmethodID Set<RefType>::_method_size = nullptr;

template <template <typename> class RefType>
jmethodID Set<RefType>::_method_contains = nullptr;

template <template <typename> class RefType>
const SimpleJNI::JNIDescriptor Set<RefType>::descriptor{
    "java/util/Set",     // Java interface name
    &_cls,               // Where to store the jclass
    {                    // Methods to preload
        {"iterator", "()Ljava/util/Iterator;", &_method_iterator},
        {"add", "(Ljava/lang/Object;)Z", &_method_add},
        {"size", "()I", &_method_size},
        {"contains", "(Ljava/lang/Object;)Z", &_method_contains}
    }
};

template <template <typename> class RefType>
const SimpleJNI::AutoRegister<Set<RefType>> Set<RefType>::registrar{&descriptor};

template <template <typename> class RefType>
Set<RefType>::Set() : _obj() {}

template <template <typename> class RefType>
Set<RefType>::Set(jobject obj) : _obj(obj) {
    if (!_cls.get()) {
        throw std::runtime_error("Set JNI resources not preloaded");
    }
    _obj = SimpleJNI::Object<RefType, jobject>(obj, _cls.get());
}

template <template <typename> class RefType>
template <template <typename> class OtherRefType>
Set<RefType>::Set(const SimpleJNI::Object<OtherRefType, jobject>& obj) : _obj(obj.get()) {}

template <template <typename> class RefType>
Set<SimpleJNI::LocalRef> Set<RefType>::to_local() const {
    if (!*this) return Set<SimpleJNI::LocalRef>();
    return Set<SimpleJNI::LocalRef>(_obj.get());
}

template <template <typename> class RefType>
Set<SimpleJNI::GlobalRef> Set<RefType>::to_global() const {
    if (!*this) return Set<SimpleJNI::GlobalRef>();
    return Set<SimpleJNI::GlobalRef>(_obj.get());
}

template <template <typename> class RefType>
jobject Set<RefType>::get() const {
    return _obj.get();
}

template <template <typename> class RefType>
Set<RefType>::operator bool() const {
    return _obj.get() != nullptr;
}

template <template <typename> class RefType>
Iterator<SimpleJNI::LocalRef> Set<RefType>::iterator() const {
    if (!*this) {
        throw std::runtime_error("Set is not initialized");
    }
    jobject iter = _obj.call_object_method(_method_iterator).get();
    return Iterator<SimpleJNI::LocalRef>(iter);
}

template <template <typename> class RefType>
bool Set<RefType>::add(const SimpleJNI::Object<RefType, jobject>& element) {
    if (!*this) {
        throw std::runtime_error("Set is not initialized");
    }
    return _obj.call_boolean_method(_method_add, element.get());
}

template <template <typename> class RefType>
size_t Set<RefType>::size() const {
    if (!*this) {
        throw std::runtime_error("Set is not initialized");
    }
    return static_cast<size_t>(_obj.call_int_method(_method_size));
}

template <template <typename> class RefType>
bool Set<RefType>::contains(const SimpleJNI::Object<RefType, jobject>& element) const {
    if (!*this) {
        throw std::runtime_error("Set is not initialized");
    }
    return _obj.call_boolean_method(_method_contains, element.get());
}

template <template <typename> class RefType>
Set<RefType>::operator SimpleJNI::Object<RefType, jobject>() const {
    return _obj;
}

// Explicit template instantiations
template class Set<SimpleJNI::LocalRef>;
template class Set<SimpleJNI::GlobalRef>;
template class Set<SimpleJNI::WeakRef>;
template class Set<SimpleJNI::ReleasableLocalRef>;

}  // namespace Java::Util