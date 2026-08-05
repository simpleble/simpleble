#pragma once

#include <string>

#include "simplejni/References.hpp"
#include "simplejni/Registry.hpp"

namespace Org {
namespace SimpleBLE {
namespace Android {

class SimpleDroidBleException {
  public:
    static void throw_new(JNIEnv* env, const std::string& message) noexcept;

  private:
    static SimpleJNI::GlobalRef<jclass> _cls;
    static const SimpleJNI::JNIDescriptor descriptor;
    static const SimpleJNI::AutoRegister<SimpleDroidBleException> registrar;
};

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
