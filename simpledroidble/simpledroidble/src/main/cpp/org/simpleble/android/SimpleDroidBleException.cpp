#include "SimpleDroidBleException.h"

#include <simpleble/Logging.h>

namespace Org {
namespace SimpleBLE {
namespace Android {

SimpleJNI::GlobalRef<jclass> SimpleDroidBleException::_cls;

const SimpleJNI::JNIDescriptor SimpleDroidBleException::descriptor{
    "org/simpleble/android/SimpleDroidBleException", &_cls, {}};

const SimpleJNI::AutoRegister<SimpleDroidBleException> SimpleDroidBleException::registrar{&descriptor};

void SimpleDroidBleException::throw_new(JNIEnv* env, const std::string& message) noexcept {
    ::SimpleBLE::Logging::Logger::get()->log(::SimpleBLE::Logging::Level::Error, "SimpleDroidBLE", __FILE__,
                                             __LINE__, __func__, "Throwing exception: " + message);

    if (_cls.get()) env->ThrowNew(_cls.get(), message.c_str());
}

}  // namespace Android
}  // namespace SimpleBLE
}  // namespace Org
