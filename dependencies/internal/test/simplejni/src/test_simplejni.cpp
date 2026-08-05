#include <jni.h>

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "simplejni/Common.hpp"
#include "simplejni/VM.hpp"

namespace {

using SimpleJNI::ByteArray;
using SimpleJNI::Exception;
using SimpleJNI::GlobalRef;
using SimpleJNI::LocalRef;
using SimpleJNI::LongArray;
using SimpleJNI::Object;
using SimpleJNI::String;
using SimpleJNI::VM;
using SimpleJNI::WeakRef;
using SimpleJNI::adopt_local_ref;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

class LocalFrame {
  public:
    LocalFrame(JNIEnv* env, jint capacity) : _env(env) {
        if (_env->PushLocalFrame(capacity) != JNI_OK) {
            Exception::check(_env);
            throw std::runtime_error("Failed to create JNI local frame");
        }
    }

    ~LocalFrame() { _env->PopLocalFrame(nullptr); }

    LocalFrame(const LocalFrame&) = delete;
    LocalFrame& operator=(const LocalFrame&) = delete;

  private:
    JNIEnv* _env;
};

struct Methods {
    jmethodID constructor;
    jmethodID value;
    jmethodID truth;
    jmethodID echo_object;
    jmethodID echo_string;
    jmethodID echo_bytes;
    jmethodID throw_exception;
    jmethodID throw_broken_exception;
};

jmethodID get_method(JNIEnv* env, jclass cls, const char* name, const char* signature) {
    jmethodID method = env->GetMethodID(cls, name, signature);
    Exception::check(env);
    require(method != nullptr, std::string("Failed to resolve method: ") + name);
    return method;
}

Object<LocalRef> new_target(JNIEnv* env, jclass cls, const Methods& methods, jint value) {
    jobject target = env->NewObject(cls, methods.constructor, value);
    Exception::check(env);
    require(target != nullptr, "Failed to create TestTarget");
    return Object<LocalRef>(adopt_local_ref, target);
}

class Harness {
  public:
    void run(const char* name, const std::function<void()>& test) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& exception) {
            ++_failures;
            std::cerr << "[FAIL] " << name << ": " << exception.what() << '\n';
        }
    }

    int failures() const { return _failures; }

  private:
    int _failures = 0;
};

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Expected the Java test classes directory\n";
        return 1;
    }

    std::string class_path = "-Djava.class.path=" + std::string(argv[1]);
    std::string check_jni = "-Xcheck:jni";
    std::string max_jni_locals = "-XX:MaxJNILocalCapacity=256";
    JavaVMOption options[] = {{class_path.data()}, {check_jni.data()}, {max_jni_locals.data()}};
    JavaVMInitArgs vm_args{};
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 3;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_FALSE;

    JavaVM* vm = nullptr;
    JNIEnv* env = nullptr;
    if (JNI_CreateJavaVM(&vm, reinterpret_cast<void**>(&env), &vm_args) != JNI_OK) {
        std::cerr << "Failed to create the JVM\n";
        return 1;
    }
    VM::jvm(vm);

    int failures = 0;
    {
        jclass local_test_class = env->FindClass("org/simplejni/TestTarget");
        Exception::check(env);
        require(local_test_class != nullptr, "Failed to load TestTarget");
        GlobalRef<jclass> test_class(adopt_local_ref, local_test_class);

        const Methods methods{
            get_method(env, test_class.get(), "<init>", "(I)V"),
            get_method(env, test_class.get(), "value", "()I"),
            get_method(env, test_class.get(), "truth", "()Z"),
            get_method(env, test_class.get(), "echoObject", "(Ljava/lang/Object;)Ljava/lang/Object;"),
            get_method(env, test_class.get(), "echoString", "(Ljava/lang/String;)Ljava/lang/String;"),
            get_method(env, test_class.get(), "echoBytes", "([B)[B"),
            get_method(env, test_class.get(), "throwException", "()V"),
            get_method(env, test_class.get(), "throwBrokenException", "()V"),
        };

        Harness harness;

        harness.run("borrowed and adopted references", [&] {
            LocalFrame frame(env, 16);

            jobject borrowed_raw = env->NewObject(test_class.get(), methods.constructor, 7);
            Exception::check(env);
            Object<LocalRef> borrowed(borrowed_raw);
            env->DeleteLocalRef(borrowed_raw);
            require(borrowed.call_int_method(methods.value) == 7, "Borrowed local reference was not cloned");

            jobject adopted_raw = env->NewObject(test_class.get(), methods.constructor, 8);
            Exception::check(env);
            Object<LocalRef> adopted(adopt_local_ref, adopted_raw);
            require(adopted.call_int_method(methods.value) == 8, "Adopted local reference is invalid");

            jobject global_raw = env->NewObject(test_class.get(), methods.constructor, 9);
            Exception::check(env);
            Object<GlobalRef> global(adopt_local_ref, global_raw);
            require(global.to_local().call_int_method(methods.value) == 9, "Adopted global reference is invalid");

            jobject weak_raw = env->NewObject(test_class.get(), methods.constructor, 10);
            Exception::check(env);
            Object<LocalRef> weak_keeper(weak_raw);
            Object<WeakRef> weak(adopt_local_ref, weak_raw);
            require(weak.is_valid(), "Adopted weak reference is invalid");
            require(weak.to_local().call_int_method(methods.value) == 10, "Weak reference cannot be promoted");
        });

        harness.run("object and value conversions", [&] {
            LocalFrame frame(env, 16);
            auto target = new_target(env, test_class.get(), methods, 42);
            require(target.call_int_method(methods.value) == 42, "Integer method returned the wrong value");
            require(target.call_boolean_method(methods.truth), "Boolean method returned the wrong value");

            String<LocalRef> string("SimpleJNI");
            auto echoed_object = target.call_object_method(methods.echo_object, string.get());
            require(env->IsSameObject(echoed_object.get(), string.get()), "Object method returned a different object");
            require(target.call_string_method(methods.echo_string, string.get()) == "SimpleJNI",
                    "String method returned the wrong value");

            const kvn::bytearray bytes{0x01, 0x02, 0x03};
            ByteArray<LocalRef> byte_array(bytes);
            require(target.call_byte_array_method(methods.echo_bytes, byte_array.get()).toHex() == bytes.toHex(),
                    "Byte array method returned the wrong value");

            const std::vector<int64_t> longs{1, 2, 3};
            LongArray<LocalRef> long_array(longs);
            require(long_array.longs() == longs, "Long array conversion returned the wrong value");
        });

        harness.run("exception translation", [&] {
            LocalFrame frame(env, 16);
            auto target = new_target(env, test_class.get(), methods, 0);
            std::exception_ptr translated;

            try {
                target.call_void_method(methods.throw_exception);
                throw std::runtime_error("Expected Java exception was not translated");
            } catch (const Exception& exception) {
                require(std::string(exception.what()) ==
                            "Java Exception: java.lang.IllegalStateException: callback failed",
                        "Translated exception has the wrong message");
                translated = std::current_exception();
            }

            require(!env->ExceptionCheck(), "Translated exception remained pending in Java");

            std::exception_ptr worker_failure;
            std::thread worker([translated = std::move(translated), &worker_failure]() mutable {
                try {
                    std::rethrow_exception(translated);
                } catch (const Exception& exception) {
                    if (std::string(exception.what()) !=
                        "Java Exception: java.lang.IllegalStateException: callback failed") {
                        worker_failure = std::make_exception_ptr(
                            std::runtime_error("Translated exception changed across threads"));
                    }
                } catch (...) {
                    worker_failure = std::current_exception();
                }
                translated = nullptr;
            });
            worker.join();
            if (worker_failure) std::rethrow_exception(worker_failure);
        });

        harness.run("exception formatting failure", [&] {
            LocalFrame frame(env, 16);
            auto target = new_target(env, test_class.get(), methods, 0);

            try {
                target.call_void_method(methods.throw_broken_exception);
                throw std::runtime_error("Expected Java exception was not translated");
            } catch (const Exception& exception) {
                require(std::string(exception.what()) == "Java Exception",
                        "Exception formatting failure did not use the fallback message");
            }

            require(!env->ExceptionCheck(), "Exception from Throwable.toString remained pending in Java");
        });

        harness.run("local reference stress", [&] {
            LocalFrame frame(env, 16);
            const kvn::bytearray bytes{0x01, 0x02, 0x03};
            const std::vector<int64_t> longs{1, 2, 3};

            for (int i = 0; i < 1000; ++i) {
                String<LocalRef> string("SimpleJNI");
                ByteArray<LocalRef> byte_array(bytes);
                LongArray<LocalRef> long_array(longs);
                auto target = new_target(env, test_class.get(), methods, i);

                require(string.str() == "SimpleJNI", "String conversion failed during stress test");
                require(byte_array.bytes().toHex() == bytes.toHex(), "Byte conversion failed during stress test");
                require(long_array.longs() == longs, "Long conversion failed during stress test");
                require(target.call_int_method(methods.value) == i, "Object failed during stress test");
            }
        });

        harness.run("native thread attachment", [&] {
            std::exception_ptr worker_failure;
            std::thread worker([&] {
                try {
                    JNIEnv* worker_env = VM::env();
                    jclass object_class = worker_env->FindClass("java/lang/Object");
                    Exception::check(worker_env);
                    require(object_class != nullptr, "Attached thread could not use JNI");
                    worker_env->DeleteLocalRef(object_class);
                    VM::detach();
                } catch (...) {
                    worker_failure = std::current_exception();
                }
            });
            worker.join();
            if (worker_failure) std::rethrow_exception(worker_failure);
        });

        failures = harness.failures();
    }

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        failures = failures == 0 ? 1 : failures;
    }

    if (vm->DestroyJavaVM() != JNI_OK) {
        std::cerr << "Failed to destroy the JVM\n";
        return 1;
    }

    return failures == 0 ? 0 : 1;
}
