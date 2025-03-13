#pragma once

#include <jni.h>
#include <cstddef>
#include "jni/Common.hpp"
#include "jni/Registry.hpp"
#include "Iterator.h"

namespace Java::Util {

template <template <typename> class RefType>
class Set {
public:
    Set();
    explicit Set(jobject obj);

    template <template <typename> class OtherRefType>
    Set(const SimpleJNI::Object<OtherRefType, jobject>& obj);

    // Conversion methods
    Set<SimpleJNI::LocalRef> to_local() const;
    Set<SimpleJNI::GlobalRef> to_global() const;

    // Get the underlying jobject
    jobject get() const;

    // Check if the object is valid
    explicit operator bool() const;

    // Implicit conversion to SimpleJNI::Object
    operator SimpleJNI::Object<RefType, jobject>() const;

    // Set methods
    Iterator<SimpleJNI::LocalRef> iterator() const;
    bool add(const SimpleJNI::Object<RefType, jobject>& element);
    size_t size() const;
    bool contains(const SimpleJNI::Object<RefType, jobject>& element) const;

private:
    // Underlying JNI object
    SimpleJNI::Object<RefType, jobject> _obj;

    // Static JNI resources
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _method_iterator;
    static jmethodID _method_add;
    static jmethodID _method_size;
    static jmethodID _method_contains;

    // JNI descriptor for auto-registration
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<Set> registrar;
};

}  // namespace Java::Util