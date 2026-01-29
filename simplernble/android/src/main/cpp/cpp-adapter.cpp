#include <jni.h>
#include <simpleble/Advanced.h>
#include "NitroSimplernbleOnLoad.hpp"

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    // Initialize SimpleBLE's JVM reference BEFORE any SimpleBLE calls
    SimpleBLE::Advanced::Android::set_jvm(vm);

    return margelo::nitro::simplernble::initialize(vm);
}
