#pragma once

#include "jni/Common.hpp"
#include "UUID.h"

namespace SimpleBLE {
namespace Android {

class ParcelUUID {
  public:
    ParcelUUID();
    ParcelUUID(JNI::Object obj);

    UUID getUuid();

  private:
    static JNI::Class _cls;
    static jmethodID _method_getUuid;

    static void initialize();
    void check_initialized() const;
    JNI::Object _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
