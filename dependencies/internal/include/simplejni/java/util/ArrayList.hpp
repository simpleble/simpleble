#pragma once

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleJNI::Java::Util {

class ArrayList {
  public:
    ArrayList();

    void add(jobject value);
    void add(const Object<ReleasableLocalRef>& value) { add(value.get()); }
    jobject get() const { return _object.get(); }
    jobject release() { return _object.release(); }

  private:
    static jmethodID _constructor;
    static jmethodID _add;
    static const JNIDescriptor _descriptor;
    static const AutoRegister<ArrayList> _registrar;

    Object<ReleasableLocalRef> _object;
};

}  // namespace SimpleJNI::Java::Util
