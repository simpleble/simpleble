#include <jni.h>
#include "NitroSimplejsbleOnLoad.hpp"
#include <simpleble/Advanced.h>

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
  // Initialize SimpleBLE's JVM reference BEFORE any SimpleBLE calls
  SimpleBLE::Advanced::Android::set_jvm(vm);

  return margelo::nitro::simplejsble::initialize(vm);
}
