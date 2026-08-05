#pragma once

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleJNI::Java::Lang {

class Integer {
  public:
    explicit Integer(jint value);

    jobject get() const { return _object.get(); }

  private:
    static jmethodID _constructor;
    static const JNIDescriptor _descriptor;
    static const AutoRegister<Integer> _registrar;

    Object<LocalRef> _object;
};

}  // namespace SimpleJNI::Java::Lang
