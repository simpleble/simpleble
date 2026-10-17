#include "SimpleKtBleException.h"

#include <simpleble/Logging.h>

namespace Org {
namespace SimpleKtBLE {

SimpleJNI::GlobalRef<jclass> SimpleKtBleException::_cls;

const SimpleJNI::JNIDescriptor SimpleKtBleException::descriptor{
    "org/simplektble/SimpleKtBleException", &_cls, {}};

const SimpleJNI::AutoRegister<SimpleKtBleException> SimpleKtBleException::registrar{&descriptor};

void SimpleKtBleException::throw_new(JNIEnv* env, const std::string& message) noexcept {
    ::SimpleBLE::Logging::Logger::get()->log(::SimpleBLE::Logging::Level::Error, "SimpleKtBLE", __FILE__,
                                             __LINE__, __func__, "Throwing exception: " + message);

    if (_cls.get()) env->ThrowNew(_cls.get(), message.c_str());
}

}  // namespace SimpleKtBLE
}  // namespace Org
