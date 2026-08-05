#pragma once

#include <simpleble/Types.h>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleBLE {
namespace Android {

class DataCallback {
  public:
    explicit DataCallback(jobject callback);

    void on_data_received(const ::SimpleBLE::ByteArray& data) const noexcept;

  private:
    static jmethodID _method_on_data_received;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<DataCallback> registrar;

    SimpleJNI::Object<SimpleJNI::GlobalRef> _obj;
};

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
