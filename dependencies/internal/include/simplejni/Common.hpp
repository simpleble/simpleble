#pragma once

#include <jni.h>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "References.hpp"
#include "Registry.hpp"
#include "VM.hpp"
#include "kvn/kvn_bytearray.h"

namespace SimpleJNI {

// TODO: Implement a base class that handles common functionality of complicated Objects (see the ones in
// org/simplejavable)

class Exception : public std::runtime_error {
  public:
    Exception(AdoptLocalRefTag, jthrowable obj) : std::runtime_error("Java Exception"), _what(describe(obj)) {}

    const char* what() const noexcept override { return _what.c_str(); }

    static void check(JNIEnv* env) {
        jthrowable throwable = env->ExceptionOccurred();
        if (!throwable) return;

        env->ExceptionClear();
        throw Exception(adopt_local_ref, throwable);
    }

  private:
    std::string _what;

    static std::string describe(jthrowable obj) {
        LocalRef<jthrowable> ref(adopt_local_ref, obj);
        if (!ref.get()) return "Java Exception";

        JNIEnv* env = SimpleJNI::VM::env();
        LocalRef<jclass> cls(adopt_local_ref, env->GetObjectClass(ref.get()));
        if (!cls.get()) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            return "Java Exception";
        }

        std::string what = "Java Exception";
        jmethodID method_to_string = env->GetMethodID(cls.get(), "toString", "()Ljava/lang/String;");
        if (!method_to_string) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            return what;
        }

        LocalRef<jstring> jstr(adopt_local_ref,
                               static_cast<jstring>(env->CallObjectMethod(ref.get(), method_to_string)));
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            return what;
        }
        if (!jstr.get()) return what;

        const char* c_str = env->GetStringUTFChars(jstr.get(), nullptr);
        if (!c_str) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            return what;
        }

        what += ": ";
        what += c_str;
        env->ReleaseStringUTFChars(jstr.get(), c_str);
        return what;
    }
};

template <template <typename> class RefType, typename JniType = jobject>
class Object {
  public:
    Object() = default;

    explicit Object(JniType obj) : _ref(obj) {}

    Object(AdoptLocalRefTag, JniType obj) : _ref(adopt_local_ref, obj) {}

    // Move semantics
    Object(Object&& other) noexcept : _ref(std::move(other._ref)) {}
    Object& operator=(Object&& other) noexcept {
        if (this != &other) {
            _ref = std::move(other._ref);
        }
        return *this;
    }

    // Copying depends on RefType's behavior (enabled by default)
    Object(const Object&) = default;
    Object& operator=(const Object&) = default;

    // Template constructor for converting between different RefType templates
    template <template <typename> class OtherRefType>
    Object(const Object<OtherRefType, JniType>& other) : _ref(other.get()) {}

    // Template assignment operator for converting between different RefType templates
    template <template <typename> class OtherRefType>
    Object& operator=(const Object<OtherRefType, JniType>& other) {
        // Don't use pointer comparison for different template types
        // Instead, check if the underlying JNI objects are the same
        JNIEnv* env = VM::env();

        // Only proceed with assignment if the objects are different
        // or if other is null (in which case we reset this object)
        if (!other.get() || !_ref.get() || !env->IsSameObject(_ref.get(), other.get())) {
            if (other.get()) {
                _ref = RefType<JniType>(other.get());
            } else {
                _ref = RefType<JniType>();
            }
        }
        return *this;
    }

    // Conversion methods
    Object<LocalRef, JniType> to_local() const {
        if (!*this) return Object<LocalRef, JniType>();
        return Object<LocalRef, JniType>(_ref.get());
    }

    Object<GlobalRef, JniType> to_global() const {
        if (!*this) return Object<GlobalRef, JniType>();
        return Object<GlobalRef, JniType>(_ref.get());
    }

    Object<WeakRef, jweak> to_weak() const {
        if (!*this) return Object<WeakRef, jweak>();
        return Object<WeakRef, jweak>(static_cast<jweak>(_ref.get()));
    }

    // Access raw jobject
    JniType get() const { return _ref.get(); }

    // Release ownership of the underlying reference
    JniType release() noexcept { return _ref.release(); }

    explicit operator bool() const { return _ref.get() != nullptr; }

    bool is_valid() const { return _ref.is_valid(); }

    template <typename... Args>
    Object<LocalRef, JniType> call_object_method(jmethodID method, Args&&... args) const {
        JNIEnv* env = VM::env();
        JniType result = env->CallObjectMethod(_ref.get(), method, std::forward<Args>(args)...);
        Exception::check(env);
        return Object<LocalRef, JniType>(adopt_local_ref, result);
    }

    template <typename... Args>
    void call_void_method(jmethodID method, Args&&... args) const {
        JNIEnv* env = VM::env();
        env->CallVoidMethod(_ref.get(), method, std::forward<Args>(args)...);
        Exception::check(env);
    }

    template <typename... Args>
    bool call_boolean_method(jmethodID method, Args&&... args) const {
        JNIEnv* env = VM::env();
        bool result = env->CallBooleanMethod(_ref.get(), method, std::forward<Args>(args)...);
        Exception::check(env);
        return result;
    }

    template <typename... Args>
    int call_int_method(jmethodID method, Args&&... args) const {
        JNIEnv* env = VM::env();
        int result = env->CallIntMethod(_ref.get(), method, std::forward<Args>(args)...);
        Exception::check(env);
        return result;
    }

    template <typename... Args>
    std::string call_string_method(jmethodID method, Args&&... args) const {
        JNIEnv* env = VM::env();
        LocalRef<jstring> jstr(adopt_local_ref, static_cast<jstring>(env->CallObjectMethod(
                                                    _ref.get(), method, std::forward<Args>(args)...)));
        Exception::check(env);
        if (!jstr.get()) return "";
        const char* c_str = env->GetStringUTFChars(jstr.get(), nullptr);
        Exception::check(env);
        std::string result(c_str);
        env->ReleaseStringUTFChars(jstr.get(), c_str);
        return result;
    }

    template <typename... Args>
    kvn::bytearray call_byte_array_method(jmethodID method, Args&&... args) const {
        JNIEnv* env = VM::env();
        LocalRef<jbyteArray> jarr(adopt_local_ref, static_cast<jbyteArray>(env->CallObjectMethod(
                                                       _ref.get(), method, std::forward<Args>(args)...)));
        Exception::check(env);
        if (!jarr.get()) return {};
        jsize len = env->GetArrayLength(jarr.get());
        Exception::check(env);
        kvn::bytearray result(static_cast<size_t>(len));
        env->GetByteArrayRegion(jarr.get(), 0, len, reinterpret_cast<jbyte*>(result.data()));
        Exception::check(env);
        return result;
    }

    template <typename... Args>
    static Object<LocalRef, JniType> call_new_object(jclass cls, jmethodID method, Args&&... args) {
        JNIEnv* env = VM::env();
        JniType result = env->NewObject(cls, method, std::forward<Args>(args)...);
        Exception::check(env);
        return Object<LocalRef, JniType>(adopt_local_ref, result);
    }

  protected:
    RefType<JniType> _ref;
};

template <template <typename> class RefType>
class ByteArray {
  public:
    ByteArray() = default;

    // NOTE: The user is responsible for ensuring that the jobject is a jbyteArray
    explicit ByteArray(jobject obj) : _ref(static_cast<jbyteArray>(obj)) {}

    explicit ByteArray(jbyteArray obj) : _ref(obj) {}

    ByteArray(const kvn::bytearray& data) : _ref() {
        JNIEnv* env = VM::env();
        jbyteArray jarr = env->NewByteArray(data.size());
        Exception::check(env);
        env->SetByteArrayRegion(jarr, 0, data.size(), reinterpret_cast<const jbyte*>(data.data()));
        Exception::check(env);

        this->_ref = RefType<jbyteArray>(adopt_local_ref, jarr);
    }

    template <template <typename> class OtherRefType>
    ByteArray(const Object<OtherRefType, jbyteArray>& obj) : _ref(obj.get()) {}

    // Add implicit conversion to Object<RefType, jobject>
    operator Object<RefType, jobject>() const { return Object<RefType, jobject>(static_cast<jobject>(this->get())); }

    // Access raw jobject
    jbyteArray get() const { return _ref.get(); }

    // Release ownership of the underlying reference
    jbyteArray release() noexcept { return _ref.release(); }

    // Conversion methods
    ByteArray<LocalRef> to_local() const {
        if (!*this) return ByteArray<LocalRef>();
        return ByteArray<LocalRef>(this->get());
    }

    ByteArray<GlobalRef> to_global() const {
        if (!*this) return ByteArray<GlobalRef>();
        return ByteArray<GlobalRef>(this->get());
    }

    explicit operator bool() const { return _ref.get() != nullptr; }

    // Get the raw byte array data
    kvn::bytearray bytes() const {
        JNIEnv* env = VM::env();
        jbyteArray jarr = this->get();

        if (jarr == nullptr) {
            return {};
        }

        jsize len = env->GetArrayLength(jarr);
        Exception::check(env);
        kvn::bytearray result(len);

        env->GetByteArrayRegion(jarr, 0, len, reinterpret_cast<jbyte*>(result.data()));
        Exception::check(env);

        return result;
    }

    // Get the length of the byte array
    size_t length() const {
        JNIEnv* env = VM::env();
        return env->GetArrayLength(this->get());
    }

  protected:
    RefType<jbyteArray> _ref;
};

template <template <typename> class RefType>
class LongArray {
  public:
    LongArray() = default;

    explicit LongArray(jlongArray obj) : _ref(obj) {}

    LongArray(const std::vector<int64_t>& data) : _ref() {
        JNIEnv* env = VM::env();
        jlongArray jarr = env->NewLongArray(data.size());
        Exception::check(env);
        env->SetLongArrayRegion(jarr, 0, data.size(), reinterpret_cast<const jlong*>(data.data()));
        Exception::check(env);

        this->_ref = RefType<jlongArray>(adopt_local_ref, jarr);
    }

    template <template <typename> class OtherRefType>
    LongArray(const Object<OtherRefType, jlongArray>& obj) : _ref(obj.get()) {}

    // Add implicit conversion to Object<RefType, jobject>
    operator Object<RefType, jobject>() const { return Object<RefType, jobject>(static_cast<jobject>(this->get())); }

    // Access raw jobject
    jlongArray get() const { return _ref.get(); }

    // Release ownership of the underlying reference
    jlongArray release() noexcept { return _ref.release(); }

    // Conversion methods
    LongArray<LocalRef> to_local() const {
        if (!*this) return LongArray<LocalRef>();
        return LongArray<LocalRef>(this->get());
    }

    LongArray<GlobalRef> to_global() const {
        if (!*this) return LongArray<GlobalRef>();
        return LongArray<GlobalRef>(this->get());
    }

    explicit operator bool() const { return _ref.get() != nullptr; }

    // Get the raw long array data
    std::vector<int64_t> longs() const {
        JNIEnv* env = VM::env();
        jlongArray jarr = this->get();

        if (jarr == nullptr) {
            return {};
        }

        jsize len = env->GetArrayLength(jarr);
        Exception::check(env);
        std::vector<int64_t> result(len);

        env->GetLongArrayRegion(jarr, 0, len, reinterpret_cast<jlong*>(result.data()));
        Exception::check(env);

        return result;
    }

    // Get the length of the long array
    size_t length() const {
        JNIEnv* env = VM::env();
        jlongArray jarr = this->get();

        if (jarr == nullptr) {
            return 0;  // Return 0 for null arrays
        }

        return env->GetArrayLength(jarr);
    }

  protected:
    RefType<jlongArray> _ref;
};

template <template <typename> class RefType>
class String {
  public:
    String() = default;

    explicit String(jstring obj) : _ref(obj) {}

    String(const std::string& data) : _ref() {
        JNIEnv* env = VM::env();
        jstring jstr = env->NewStringUTF(data.c_str());
        Exception::check(env);

        this->_ref = RefType<jstring>(adopt_local_ref, jstr);
    }

    template <template <typename> class OtherRefType>
    String(const Object<OtherRefType, jstring>& obj) : _ref(obj.get()) {}

    // Add implicit conversion to Object<RefType, jobject>
    operator Object<RefType, jobject>() const { return Object<RefType, jobject>(static_cast<jobject>(this->get())); }

    explicit operator bool() const { return _ref.get() != nullptr; }

    // Access raw jobject
    jstring get() const { return _ref.get(); }

    // Release ownership of the underlying reference
    jstring release() noexcept { return _ref.release(); }

    // Conversion methods
    String<LocalRef> to_local() const {
        if (!*this) return String<LocalRef>();
        return String<LocalRef>(this->get());
    }

    String<GlobalRef> to_global() const {
        if (!*this) return String<GlobalRef>();
        return String<GlobalRef>(this->get());
    }

    // Get the raw string data
    std::string str() const {
        JNIEnv* env = VM::env();
        jstring jstr = this->get();

        if (jstr == nullptr) {
            return {};
        }

        const char* c_str = env->GetStringUTFChars(jstr, nullptr);
        Exception::check(env);
        std::string result(c_str);
        env->ReleaseStringUTFChars(jstr, c_str);

        return result;
    }

    // Get the length of the string
    size_t length() const {
        JNIEnv* env = VM::env();
        return env->GetStringUTFLength(this->get());
    }

  protected:
    RefType<jstring> _ref;
};

class Env {
  public:
    Env() { _env = VM::env(); }
    virtual ~Env() = default;
    Env(Env& other) = delete;             // Remove the copy constructor
    void operator=(const Env&) = delete;  // Remove the copy assignment

    JNIEnv* operator->() { return _env; }

    operator JNIEnv*() { return _env; }

  private:
    JNIEnv* _env = nullptr;
};

class Runner {
  public:
    // Delete copy constructor and assignment
    Runner(const Runner&) = delete;
    Runner& operator=(const Runner&) = delete;

    static Runner& get() {
        static Runner instance;
        return instance;
    }

    virtual ~Runner() {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _stop = true;
            _cv.notify_one();
        }
        _thread.join();
    }

    void enqueue(std::function<void()> func) {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _queue.push(std::move(func));
            lock.unlock();
            _cv.notify_one();
        }
    }

  protected:
    void thread_func() {
        VM::attach();
        while (true) {
            std::function<void()> func;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this] { return _stop || !_queue.empty(); });
                if (_stop && _queue.empty()) {
                    break;
                }
                func = std::move(_queue.front());
                _queue.pop();
            }
            func();
        }
        VM::detach();
    }

  private:
    // Move constructor to private
    Runner() : _stop(false) { _thread = std::thread(&Runner::thread_func, this); }

    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::queue<std::function<void()>> _queue;
    bool _stop;
};

template <template <typename> class RefType, typename JniType = jobject>
struct ObjectComparator {
    bool operator()(const Object<RefType, JniType>& lhs, const Object<RefType, JniType>& rhs) const {
        // Handle null object comparisons
        if (!lhs && !rhs) {
            return false;  // Both are null, considered equal
        }
        if (!lhs) {
            return true;  // lhs is null, rhs is not, lhs < rhs
        }
        if (!rhs) {
            return false;  // rhs is null, lhs is not, lhs > rhs
        }

        JNIEnv* env = VM::env();

        // Access the underlying jobject handles from Object instances
        JniType lhsObject = lhs.get();
        JniType rhsObject = rhs.get();

        // Check if both jobject handles refer to the same object
        if (env->IsSameObject(lhsObject, rhsObject)) {
            return false;  // Both objects are the same
        }

        jmethodID method_hashCode = Registrar::get().get_method("java/lang/Object", "hashCode");
        jint lhsHashCode = env->CallIntMethod(lhsObject, method_hashCode);
        jint rhsHashCode = env->CallIntMethod(rhsObject, method_hashCode);

        if (lhsHashCode != rhsHashCode) {
            return lhsHashCode < rhsHashCode;  // Use hash code for initial comparison
        }

        // Use a direct pointer comparison as a fallback for objects with identical hash codes
        return lhsObject < rhsObject;  // This comparison is consistent within the same execution
    }
};
}  // namespace SimpleJNI
