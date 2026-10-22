#pragma once

#include <jni.h>

#include <memory>

#include "simplejni/ThreadRunner.hpp"

/**
 * The thread that runs every Java event callback, one at a time and in order.
 *
 * SimpleBLE's threads only queue events here, so they never run Java code. At exit the JVM stops any thread that is
 * running Java code, and SimpleBLE waits for its own threads, so a SimpleBLE thread inside a Java callback would hang
 * the process.
 *
 * NOTE: Never destroyed. At exit its thread may be stuck in the JVM, so joining it from a static destructor hangs.
 */
inline SimpleJNI::ThreadRunner& callback_runner() {
    static SimpleJNI::ThreadRunner& runner = *new SimpleJNI::ThreadRunner("SimpleJavaBLE");
    return runner;
}

/**
 * Wraps a Java callback object for SimpleBLE's callbacks to hold.
 *
 * SimpleBLE may release its callbacks on its own threads, which are not attached to the JVM and so cannot release a
 * Java reference. The wrapper is deleted on the callback runner instead.
 */
template <typename T>
std::shared_ptr<T> make_java_callback(jobject callback) {
    return std::shared_ptr<T>(new T(callback),
                              [](T* wrapper) { callback_runner().enqueue([wrapper] { delete wrapper; }); });
}
