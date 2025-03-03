#pragma once

#include <jni.h>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "References.hpp"
#include "VM.hpp"
#include "external/kvn_bytearray.h"

namespace SimpleJNI {

template <template <typename> class RefType, typename JniType = jobject>
class Object {
  public:
    Object() = default;

    explicit Object(JniType obj) : _ref(obj) {
        if (obj) {
            JNIEnv* env = VM::env();
            _cls = RefType<jclass>(env->GetObjectClass(_ref.get()));
        }
    }

    // Constructor with pre-fetched jclass
    Object(JniType obj, jclass cls) : _ref(obj), _cls(cls) {}

    // Move semantics
    Object(Object&& other) noexcept : _ref(std::move(other._ref)), _cls(std::move(other._cls)) {}
    Object& operator=(Object&& other) noexcept {
        if (this != &other) {
            _ref = std::move(other._ref);
            _cls = std::move(other._cls);
        }
        return *this;
    }

    // Copying depends on RefType's behavior (enabled by default)
    Object(const Object&) = default;
    Object& operator=(const Object&) = default;

    // Conversion methods
    Object<LocalRef, JniType> to_local() const {
        if (!*this) return Object<LocalRef, JniType>();
        JNIEnv* env = VM::env();
        return Object<LocalRef, JniType>(_ref.get(), _cls.get());
    }

    Object<GlobalRef, JniType> to_global() const {
        if (!*this) return Object<GlobalRef, JniType>();
        JNIEnv* env = VM::env();
        return Object<GlobalRef, JniType>(_ref.get(), _cls.get());
    }

    Object<WeakRef, jweak> to_weak() const {
        if (!*this) return Object<WeakRef, jweak>();
        JNIEnv* env = VM::env();
        return Object<WeakRef, jweak>(WeakRef<jweak>(_ref.get()));
    }

    // Access raw jobject
    JniType get() const { return _ref.get(); }

    // Release ownership of the underlying reference
    JniType release() noexcept { return _ref.release(); }

    explicit operator bool() const { return _ref.get() != nullptr; }

    jmethodID get_method(const char* name, const char* signature) {
        JNIEnv* env = VM::env();
        return env->GetMethodID(_cls.get(), name, signature);
    }

    template <typename... Args>
    Object<LocalRef, JniType> call_object_method(jmethodID method, Args&&... args) {
        JNIEnv* env = VM::env();
        JniType result = env->CallObjectMethod(_ref.get(), method, std::forward<Args>(args)...);
        return Object<LocalRef, JniType>(result);
    }

    template <typename... Args>
    void call_void_method(jmethodID method, Args&&... args) {
        JNIEnv* env = VM::env();
        env->CallVoidMethod(_ref.get(), method, std::forward<Args>(args)...);
    }

    template <typename... Args>
    bool call_boolean_method(jmethodID method, Args&&... args) {
        JNIEnv* env = VM::env();
        return env->CallBooleanMethod(_ref.get(), method, std::forward<Args>(args)...);
    }

    template <typename... Args>
    int call_int_method(jmethodID method, Args&&... args) {
        JNIEnv* env = VM::env();
        return env->CallIntMethod(_ref.get(), method, std::forward<Args>(args)...);
    }

    template <typename... Args>
    std::string call_string_method(jmethodID method, Args&&... args) {
        JNIEnv* env = VM::env();
        auto jstr = static_cast<jstring>(env->CallObjectMethod(_ref.get(), method, std::forward<Args>(args)...));
        if (!jstr) return "";
        const char* c_str = env->GetStringUTFChars(jstr, nullptr);
        std::string result(c_str);
        env->ReleaseStringUTFChars(jstr, c_str);
        return result;
    }

    template <typename... Args>
    kvn::bytearray call_byte_array_method(jmethodID method, Args&&... args) {
        JNIEnv* env = VM::env();
        auto jarr = static_cast<jbyteArray>(env->CallObjectMethod(_ref.get(), method, std::forward<Args>(args)...));
        if (!jarr) return {};
        jsize len = env->GetArrayLength(jarr);
        jbyte* arr = env->GetByteArrayElements(jarr, nullptr);
        kvn::bytearray result(arr, arr + len);
        env->ReleaseByteArrayElements(jarr, arr, JNI_ABORT);
        return result;
    }

  protected:
    RefType<JniType> _ref;  // Holds LocalRef<JniType> or GlobalRef<JniType>
    RefType<jclass> _cls;   // Holds LocalRef<jclass> or GlobalRef<jclass>
};

template <template <typename> class RefType>
class ByteArray : public Object<RefType, jbyteArray> {
  public:
    ByteArray() = default;

    explicit ByteArray(jbyteArray obj) : Object<RefType, jbyteArray>(obj) {}

    ByteArray(const kvn::bytearray& data) : Object<RefType, jbyteArray>() {
        JNIEnv* env = VM::env();
        jbyteArray jarr = env->NewByteArray(data.size());
        env->SetByteArrayRegion(jarr, 0, data.size(), reinterpret_cast<const jbyte*>(data.data()));

        this->_ref = RefType<jbyteArray>(jarr);
        this->_cls = RefType<jclass>(env->FindClass("java/lang/Object"));
    }

    template <template <typename> class OtherRefType>
    ByteArray(const Object<OtherRefType, jbyteArray>& obj) : Object<RefType, jbyteArray>(obj.get()) {}

    // Conversion methods
    ByteArray<LocalRef> to_local() const {
        if (!*this) return ByteArray<LocalRef>();
        return ByteArray<LocalRef>(this->get());
    }

    ByteArray<GlobalRef> to_global() const {
        if (!*this) return ByteArray<GlobalRef>();
        return ByteArray<GlobalRef>(this->get());
    }

    // Get the raw byte array data
    kvn::bytearray bytes() const {
        JNIEnv* env = VM::env();
        jbyteArray jarr = this->get();

        if (jarr == nullptr) {
            return {};
        }

        jsize len = env->GetArrayLength(jarr);
        kvn::bytearray result(len);

        env->GetByteArrayRegion(jarr, 0, len, reinterpret_cast<jbyte*>(result.data()));

        return result;
    }

    // Get the length of the byte array
    size_t length() const {
        JNIEnv* env = VM::env();
        return env->GetArrayLength(this->get());
    }
};

template <template <typename> class RefType>
class LongArray : public Object<RefType, jlongArray> {
  public:
    LongArray() = default;

    explicit LongArray(jlongArray obj) : Object<RefType, jlongArray>(obj) {}

    LongArray(const std::vector<int64_t>& data) : Object<RefType, jlongArray>() {
        JNIEnv* env = VM::env();
        jlongArray jarr = env->NewLongArray(data.size());
        env->SetLongArrayRegion(jarr, 0, data.size(), reinterpret_cast<const jlong*>(data.data()));

        this->_ref = RefType<jlongArray>(jarr);
        this->_cls = RefType<jclass>(env->FindClass("java/lang/Object"));
    }

    template <template <typename> class OtherRefType>
    LongArray(const Object<OtherRefType, jlongArray>& obj) : Object<RefType, jlongArray>(obj.get()) {}

    // Conversion methods
    LongArray<LocalRef> to_local() const {
        if (!*this) return LongArray<LocalRef>();
        return LongArray<LocalRef>(this->get());
    }

    LongArray<GlobalRef> to_global() const {
        if (!*this) return LongArray<GlobalRef>();
        return LongArray<GlobalRef>(this->get());
    }

    // Get the raw long array data
    std::vector<int64_t> longs() const {
        JNIEnv* env = VM::env();
        jlongArray jarr = this->get();

        if (jarr == nullptr) {
            return {};
        }

        jsize len = env->GetArrayLength(jarr);
        std::vector<int64_t> result(len);

        env->GetLongArrayRegion(jarr, 0, len, reinterpret_cast<jlong*>(result.data()));

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
};

template <template <typename> class RefType>
class String : public Object<RefType, jstring> {
  public:
    String() = default;

    explicit String(jstring obj) : Object<RefType, jstring>(obj) {}

    String(const std::string& data) : Object<RefType, jstring>() {
        JNIEnv* env = VM::env();
        jstring jstr = env->NewStringUTF(data.c_str());

        this->_ref = RefType<jstring>(jstr);
        this->_cls = RefType<jclass>(env->FindClass("java/lang/Object"));
    }

    template <template <typename> class OtherRefType>
    String(const Object<OtherRefType, jstring>& obj) : Object<RefType, jstring>(obj.get()) {}

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
        std::string result(c_str);
        env->ReleaseStringUTFChars(jstr, c_str);

        return result;
    }

    // Get the length of the string
    size_t length() const {
        JNIEnv* env = VM::env();
        return env->GetStringUTFLength(this->get());
    }
};


// class Class {
//   public:
//     Class() = default;
//     Class(jclass cls) : _cls(cls) {}

//     jclass get() { return _cls.get(); }

//     template <typename... Args>
//     Object call_static_method(const char* name, const char* signature, Args&&... args) {
//         JNIEnv* env = VM::env();

//         jmethodID method = env->GetStaticMethodID(_cls.get(), name, signature);
//         jobject obj = env->CallStaticObjectMethod(_cls.get(), method, std::forward<Args>(args)...);

//         return Object(obj, _cls.get());
//     }

//     template <typename... Args>
//     Object call_static_method(jmethodID method, Args&&... args) {
//         JNIEnv* env = VM::env();
//         jobject obj = env->CallStaticObjectMethod(_cls.get(), method, std::forward<Args>(args)...);
//         return Object(obj, _cls.get());
//     }

//     template <typename... Args>
//     Object call_constructor(const char* signature, Args&&... args) {
//         JNIEnv* env = VM::env();

//         jmethodID method = env->GetMethodID(_cls.get(), "<init>", signature);
//         jobject obj = env->NewObject(_cls.get(), method, std::forward<Args>(args)...);

//         return Object(obj, _cls.get());
//     }

//     void register_natives(const JNINativeMethod* methods, int nMethods) {
//         JNIEnv* env = VM::env();
//         env->RegisterNatives(_cls.get(), methods, nMethods);
//     }

//   private:
//     GlobalRef<jclass> _cls;
// };

class Env {
  public:
    Env() { _env = VM::env(); }
    virtual ~Env() = default;
    Env(Env& other) = delete;             // Remove the copy constructor
    void operator=(const Env&) = delete;  // Remove the copy assignment

    JNIEnv* operator->() { return _env; }

    // Class find_class(const std::string& name) {
    //     jclass jcls = _env->FindClass(name.c_str());
    //     if (jcls == nullptr) {
    //         throw std::runtime_error("Failed to find class: " + name);
    //     }
    //     Class cls(jcls);
    //     _env->DeleteLocalRef(jcls);
    //     return cls;
    // }

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

// // TODO: Move these to their own namespace

// struct JObjectComparator {
//     // TODO: Lazy initialize jclass and jmethodID objects.

//     bool operator()(const jobject& lhs, const jobject& rhs) const {
//         if (lhs == nullptr && rhs == nullptr) {
//             return false;  // Both are null, considered equal
//         }
//         if (lhs == nullptr) {
//             return true;  // lhs is null, rhs is not null, lhs < rhs
//         }
//         if (rhs == nullptr) {
//             return false;  // rhs is null, lhs is not null, lhs > rhs
//         }

//         JNIEnv* env = VM::env();
//         if (env->IsSameObject(lhs, rhs)) {
//             return false;  // Both objects are the same
//         }

//         // Use hashCode method to establish a consistent ordering
//         // TODO: Cache all references statically for this class!
//         jclass objectClass = env->FindClass("java/lang/Object");
//         jmethodID hashCodeMethod = env->GetMethodID(objectClass, "hashCode", "()I");

//         const jobject lhsObject = lhs;
//         const jobject rhsObject = rhs;

//         jint lhsHashCode = env->CallIntMethod(lhsObject, hashCodeMethod);
//         jint rhsHashCode = env->CallIntMethod(rhsObject, hashCodeMethod);

//         return lhsHashCode < rhsHashCode;

//         // Use a unique identifier or a pointer value as the final comparison for non-equal objects
//         return lhs < rhs;  // This can still be used for consistent ordering
//     }
// };

// struct JniObjectComparator {
//     // TODO: Lazy initialize jclass and jmethodID objects.

//     bool operator()(const Object& lhs, const Object& rhs) const {
//         // Handle null object comparisons
//         if (!lhs && !rhs) {
//             return false;  // Both are null, considered equal
//         }
//         if (!lhs) {
//             return true;  // lhs is null, rhs is not, lhs < rhs
//         }
//         if (!rhs) {
//             return false;  // rhs is null, lhs is not, lhs > rhs
//         }

//         JNIEnv* env = VM::env();

//         // Access the underlying jobject handles from Object instances
//         jobject lhsObject = lhs.get();
//         jobject rhsObject = rhs.get();

//         // Check if both jobject handles refer to the same object
//         if (env->IsSameObject(lhsObject, rhsObject)) {
//             return false;  // Both objects are the same
//         }

//         // Use hashCode method to establish a consistent ordering
//         jclass objectClass = env->FindClass("java/lang/Object");
//         jmethodID hashCodeMethod = env->GetMethodID(objectClass, "hashCode", "()I");

//         jint lhsHashCode = env->CallIntMethod(lhsObject, hashCodeMethod);
//         jint rhsHashCode = env->CallIntMethod(rhsObject, hashCodeMethod);

//         if (lhsHashCode != rhsHashCode) {
//             return lhsHashCode < rhsHashCode;  // Use hash code for initial comparison
//         }

//         // Use a direct pointer comparison as a fallback for objects with identical hash codes
//         return lhsObject < rhsObject;  // This comparison is consistent within the same execution
//     }
// };
}  // namespace SimpleJNI
