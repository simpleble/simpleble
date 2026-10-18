#include "SimpleKtBleException.h"

#include <simpleble/Logging.h>

namespace Org {
namespace SimpleKtBLE {

SimpleJNI::GlobalRef<jclass> SimpleKtBleException::_cls;
jmethodID SimpleKtBleException::_init_method = nullptr;

const SimpleJNI::JNIDescriptor SimpleKtBleException::descriptor{
    "org/simplektble/SimpleKtBleException", &_cls, {{"<init>", "(ILjava/lang/String;)V", &_init_method}}};

const SimpleJNI::AutoRegister<SimpleKtBleException> SimpleKtBleException::registrar{&descriptor};

void SimpleKtBleException::throw_new(JNIEnv* env, ::SimpleBLE::ErrorCode code, const std::string& message) noexcept {
    ::SimpleBLE::Logging::Logger::get()->log(::SimpleBLE::Logging::Level::Error, "SimpleKtBLE", __FILE__,
                                             __LINE__, __func__, "Throwing exception: " + message);

    if (!_cls.get() || !_init_method) return;

    // NOTE: A failed allocation leaves its own exception pending, which the caller then receives instead.
    jstring text = env->NewStringUTF(message.c_str());
    if (text == nullptr) return;
    auto exception = static_cast<jthrowable>(
        env->NewObject(_cls.get(), _init_method, static_cast<jint>(code), text));
    env->DeleteLocalRef(text);
    if (exception == nullptr) return;
    env->Throw(exception);
    env->DeleteLocalRef(exception);
}

}  // namespace SimpleKtBLE
}  // namespace Org
