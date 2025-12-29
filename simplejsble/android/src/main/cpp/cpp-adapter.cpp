#include <jni.h>
#include "NitroSimplejsbleOnLoad.hpp"

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
  return margelo::nitro::simplejsble::initialize(vm);
}
