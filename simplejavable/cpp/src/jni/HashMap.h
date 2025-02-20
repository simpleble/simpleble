#pragma once

#include <jni.h>
#include <vector>
#include "Common.hpp"

// TODO: Make this a template class.

namespace SimpleJNI {
namespace Types {
class HashMap {
  public:
    HashMap();
    HashMap(Object obj);

    Object put(Object key, Object value);

  private:
    static Class _cls;
    static jmethodID _method_init;
    static jmethodID _method_put;
    static void initialize();
    void check_initialized() const;
    Object _obj;
};
}  // namespace Types
}  // namespace SimpleJNI
