#if defined(__ANDROID__)

#include <jni.h>

#include <simplecble/advanced.h>
#include <simplecble/backend.h>
#include <simplecble/error.h>

namespace {

#if SIMPLEDARTBLE_BACKEND_ANDROID
// Turns a SimpleCBLE error into a Java exception, which fails the app's start loudly.
bool throw_error(JNIEnv* env, simpleble_error_t** error) {
    if (*error == nullptr) return false;
    jclass exception = env->FindClass("java/lang/IllegalStateException");
    env->ThrowNew(exception, simpleble_error_message(*error));
    simpleble_error_release(error);
    return true;
}
#endif

}  // namespace

// NOTE: The Dart VM opens this library with dlopen, which doesn't run JNI_OnLoad. SimpleDartBleInitializer loads it
// from Java first, as the app starts, so SimpleBLE's Android backend gets the JVM.
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
#if SIMPLEDARTBLE_BACKEND_ANDROID
    simpleble_error_t* error = nullptr;
    simpleble_advanced_android_set_jvm(vm, &error);
    if (error != nullptr) {
        simpleble_error_release(&error);
        return JNI_ERR;
    }
#else
    (void)vm;
#endif
    return JNI_VERSION_1_6;
}

// SimpleBLE's Android backend hosts GATT services with the application context. It also looks up its bridge classes
// when it starts, and only a thread of the app's class loader finds them, so it starts here.
extern "C" JNIEXPORT void JNICALL Java_org_simpledartble_SimpleDartBleInitializer_setContext(JNIEnv* env, jclass,
                                                                                         jobject context) {
#if SIMPLEDARTBLE_BACKEND_ANDROID
    simpleble_error_t* error = nullptr;
    simpleble_advanced_android_set_context(context, &error);
    if (throw_error(env, &error)) return;

    simpleble_backend_list_t backends = nullptr;
    simpleble_backend_get_backends(&backends, &error);
    simpleble_backend_list_release(backends);
    throw_error(env, &error);
#else
    (void)env;
    (void)context;
#endif
}

#endif
