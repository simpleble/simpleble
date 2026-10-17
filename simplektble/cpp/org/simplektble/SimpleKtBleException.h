#pragma once

#include <string>

#include "simplejni/References.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleKtBLE {

class SimpleKtBleException {
  public:
    static void throw_new(JNIEnv* env, const std::string& message) noexcept;

  private:
    static SimpleJNI::GlobalRef<jclass> _cls;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<SimpleKtBleException> registrar;
};

}  // namespace SimpleKtBLE
}  // namespace Org
