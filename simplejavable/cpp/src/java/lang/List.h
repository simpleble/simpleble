#pragma once

#include <jni.h>
#include <cstddef>
#include "jni/Common.hpp"
#include "jni/Registry.hpp"
#include "Iterator.h"

namespace Java::Util {

template <template <typename> class RefType>
class List {
public:
    List();
    explicit List(jobject obj);

    template <template <typename> class OtherRefType>
    List(const SimpleJNI::Object<OtherRefType, jobject>& obj);

    // Conversion methods
    List<SimpleJNI::LocalRef> to_local() const;
    List<SimpleJNI::GlobalRef> to_global() const;

    // Get the underlying jobject
    jobject get() const;

    // Check if the object is valid
    explicit operator bool() const;

    // Implicit conversion to SimpleJNI::Object
    operator SimpleJNI::Object<RefType, jobject>() const;

    // List methods
    Iterator<SimpleJNI::LocalRef> iterator() const;
    void add(const SimpleJNI::Object<RefType, jobject>& element);
    size_t size() const;
    SimpleJNI::Object<SimpleJNI::LocalRef> get(size_t index) const;

private:
    // Underlying JNI object
    SimpleJNI::Object<RefType, jobject> _obj;

    // Static JNI resources
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _method_iterator;
    static jmethodID _method_add;
    static jmethodID _method_size;
    static jmethodID _method_get;

    // JNI descriptor for auto-registration
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<List> registrar;
};

}  // namespace Java::Util