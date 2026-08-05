#pragma once

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace SimpleJNI::Java::Util {

class HashMap {
  public:
    HashMap();

    void put(jobject key, jobject value);
    jobject get() const { return _object.get(); }
    jobject release() { return _object.release(); }

  private:
    static jmethodID _constructor;
    static jmethodID _put;
    static const JNIDescriptor _descriptor;
    static const AutoRegister<HashMap> _registrar;

    Object<ReleasableLocalRef> _object;
};

}  // namespace SimpleJNI::Java::Util
