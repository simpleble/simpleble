#pragma once

#include <jni.h>
#include <vector>
#include "Common.hpp"

// TODO: Make this a template class.

namespace SimpleJNI {
namespace Types {
class Iterator {
  public:
    Iterator(Object obj);

    bool hasNext();
    Object next();

  private:
    static Class _cls;
    static jmethodID _method_hasNext;
    static jmethodID _method_next;

    static void initialize();
    void check_initialized() const;
    Object _obj;
};
}  // namespace Types
}  // namespace SimpleJNI