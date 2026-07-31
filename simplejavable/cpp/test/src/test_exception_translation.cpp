#include <jni.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include "simplejni/Common.hpp"
#include "simplejni/VM.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Expected the Java test classes directory\n";
        return 1;
    }

    std::string class_path = "-Djava.class.path=" + std::string(argv[1]);
    std::string check_jni = "-Xcheck:jni";
    JavaVMOption options[] = {{class_path.data()}, {check_jni.data()}};
    JavaVMInitArgs vm_args{};
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 2;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_FALSE;

    JavaVM* vm = nullptr;
    JNIEnv* env = nullptr;
    if (JNI_CreateJavaVM(&vm, reinterpret_cast<void**>(&env), &vm_args) != JNI_OK) {
        std::cerr << "Failed to create the JVM\n";
        return 1;
    }
    SimpleJNI::VM::jvm(vm);

    int result = 1;
    if (env->PushLocalFrame(8) == JNI_OK) {
        try {
            jclass callback_class = env->FindClass("org/simplejavable/ThrowingCallback");
            if (!callback_class) throw std::runtime_error("Failed to load ThrowingCallback");

            jmethodID constructor = env->GetMethodID(callback_class, "<init>", "()V");
            if (!constructor) throw std::runtime_error("Failed to load ThrowingCallback constructor");

            jobject callback_object = env->NewObject(callback_class, constructor);
            if (!callback_object) throw std::runtime_error("Failed to create ThrowingCallback");

            jmethodID on_data_received = env->GetMethodID(callback_class, "onDataReceived", "([B)V");
            if (!on_data_received) throw std::runtime_error("Failed to load onDataReceived");

            SimpleJNI::Object<SimpleJNI::LocalRef> callback(callback_object);
            jbyteArray payload = env->NewByteArray(1);
            if (!payload) throw std::runtime_error("Failed to create callback payload");

            callback.call_void_method(on_data_received, payload);
            std::cerr << "Expected the Java callback to throw\n";
        } catch (const SimpleJNI::Exception& exception) {
            const std::string expected = "Java Exception: java.lang.IllegalStateException: callback failed";
            if (exception.what() == expected && !env->ExceptionCheck()) {
                result = 0;
            } else {
                std::cerr << "Unexpected translated exception: " << exception.what() << '\n';
            }
        } catch (const std::exception& exception) {
            std::cerr << "Unexpected exception: " << exception.what() << '\n';
        }

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            result = 1;
        }
        env->PopLocalFrame(nullptr);
    } else {
        env->ExceptionClear();
        std::cerr << "Failed to create a JNI local frame\n";
    }

    if (vm->DestroyJavaVM() != JNI_OK) {
        std::cerr << "Failed to destroy the JVM\n";
        return 1;
    }
    return result;
}
