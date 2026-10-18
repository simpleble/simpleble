#pragma once

#include <jni.h>

#include <stdexcept>

#include "simplejni/VM.hpp"

namespace Org {
namespace SimpleKtBLE {

/**
 * Attaches the calling thread to the JVM as a daemon, unless it is attached already.
 *
 * A desktop JVM exits only once its non-daemon threads finish, and SimpleJNI attaches threads as non-daemons. The
 * callback thread and SimpleBLE's own threads never finish, so attached that way they keep the JVM from exiting.
 */
inline void attach_as_daemon(const char* thread_name = nullptr) {
    JavaVM* jvm = SimpleJNI::VM::jvm();
    JNIEnv* env = nullptr;
    if (jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) return;

    // OpenJDK declares the name as char*, but the JVM only reads it.
    JavaVMAttachArgs args{JNI_VERSION_1_6, const_cast<char*>(thread_name), nullptr};
#ifdef __ANDROID__
    auto result = jvm->AttachCurrentThreadAsDaemon(&env, &args);
#else
    auto result = jvm->AttachCurrentThreadAsDaemon(reinterpret_cast<void**>(&env), &args);
#endif
    if (result != JNI_OK) {
        throw std::runtime_error("Failed to attach the current thread to the JVM");
    }
}

}  // namespace SimpleKtBLE
}  // namespace Org
