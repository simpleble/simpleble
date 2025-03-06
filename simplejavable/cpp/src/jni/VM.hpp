#pragma once

#include <jni.h>

#include <mutex>

namespace SimpleJNI {

// Generic class to handle the Java Virtual Machine (JVM)
class VM {
  public:

    // TODO: Make the VM class transparent to the JavaVM pointer.
    static JavaVM* jvm() {
        static std::mutex get_mutex;       // Static mutex to ensure thread safety when accessing the VM
        static VM instance;                // Static instance of the VM to ensure proper lifecycle management

        if (instance._jvm == nullptr) {
            std::scoped_lock lock(get_mutex);  // Unlock the mutex on function return
            jsize count;
            if (JNI_GetCreatedJavaVMs(&instance._jvm, 1, &count) != JNI_OK || count == 0) {
                throw std::runtime_error("Failed to get the Java Virtual Machine");
            }
        }
        return instance._jvm;
    }

    static bool is_jvm_alive() {
        JavaVM* jvm = VM::jvm();
        if (!jvm) return false;
        JNIEnv* env = nullptr;
        return jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK;
    }

    static JNIEnv* env() {
        JNIEnv* env = nullptr;
        JavaVM* jvm = VM::jvm();

        auto getEnvResult = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        switch (getEnvResult) {
            case JNI_OK:
                break;
            case JNI_EDETACHED: {
                auto result = jvm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr);
                if (result != JNI_OK) {
                    throw std::runtime_error("Failed to attach the current thread to the JVM");
                }
                break;
            }
            case JNI_EVERSION:
                throw std::runtime_error("Failed to get the JNIEnv, JNI version not supported");
            default:
                throw std::runtime_error("Failed to get the JNIEnv, unknown error");
        }
        return env;
    }

    static void attach() {
        JNIEnv* env = nullptr;
        JavaVM* jvm = VM::jvm();
        auto result = jvm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr);
        if (result != JNI_OK) {
            throw std::runtime_error("Failed to attach the current thread to the JVM");
        }
    }

    static void detach() {
        JavaVM* jvm = VM::jvm();
        jvm->DetachCurrentThread();
    }

  private:
    VM() = default;
    virtual ~VM() = default;
    VM(VM& other) = delete;              // Remove the copy constructor
    void operator=(const VM&) = delete;  // Remove the copy assignment

    JavaVM* _jvm = nullptr;
};

}  // namespace SimpleJNI