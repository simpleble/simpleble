#pragma once

#include <jni.h>
#include <vector>
#include "Common.hpp"

// TODO: Make this a template class.

namespace SimpleJNI {
namespace Types {
class Integer {
  public:
    Integer(int value);
    Integer(Object obj);

  private:
    static Class _cls;
    static jmethodID _method_init;
    
    static void initialize();
    void check_initialized() const;
    Object _obj;
};
}  // namespace Types
}  // namespace SimpleJNI
