#pragma once

#include <jni.h>
#include <vector>
#include "Common.hpp"
#include "Iterator.h"

// TODO: Make this a template class.

namespace SimpleJNI {
namespace Types {
class Set {
  public:
    Set(Object obj);

    Iterator iterator();

  private:
    static Class _cls;
    static jmethodID _method_iterator;

    static void initialize();
    void check_initialized() const;
    Object _obj;
};
}  // namespace Types
}  // namespace SimpleJNI
