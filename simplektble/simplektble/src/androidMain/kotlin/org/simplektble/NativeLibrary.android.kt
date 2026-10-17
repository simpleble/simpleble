package org.simplektble

internal actual fun loadNativeLibrary() {
    System.loadLibrary("simplektble-jni")
}
