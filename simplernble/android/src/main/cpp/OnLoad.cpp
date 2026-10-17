#include <fbjni/fbjni.h>
#include <jni.h>
#include <simpleble/Advanced.h>
#include <simplejni/Registry.hpp>
#include <simplejni/VM.hpp>

#include "Logging.hpp"
#include "SimpleRnBLEOnLoad.hpp"

// NOTE: SimpleBLE's Android backend looks up its bridge classes while it loads.
// Only a thread that loaded this library can find app classes, so they are
// looked up here.
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;

    SimpleJNI::VM::jvm(vm);
    margelo::nitro::simplernble::log_to_system();
#if SIMPLEBLE_BACKEND_ANDROID
    SimpleBLE::Advanced::Android::set_jvm(vm);
    SimpleJNI::Registrar::get().preload(env);
#endif
    return facebook::jni::initialize(vm, [] { margelo::nitro::simplernble::registerAllNatives(); });
}

// SimpleBLE's Android backend hosts GATT services with the application context,
// which SimpleRnBLEInitializer provides when the app starts.
extern "C" JNIEXPORT void JNICALL Java_org_simplernble_SimpleRnBLEInitializer_setApplicationContext(JNIEnv*, jclass,
                                                                                                   jobject context) {
#if SIMPLEBLE_BACKEND_ANDROID
    SimpleBLE::Advanced::Android::set_context(context);
#else
    (void)context;
#endif
}
