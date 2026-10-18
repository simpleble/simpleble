#pragma once

#include <simpleble/Error.h>

#include <string>

#include "simplejni/References.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleKtBLE {

class SimpleKtBleException {
  public:
    static void throw_new(JNIEnv* env, ::SimpleBLE::ErrorCode code, const std::string& message) noexcept;

  private:
    static SimpleJNI::GlobalRef<jclass> _cls;
    static jmethodID _init_method;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<SimpleKtBleException> registrar;
};

}  // namespace SimpleKtBLE
}  // namespace Org
