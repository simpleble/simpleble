#pragma once

#include <napi.h>
#include <simpleble/Backend.h>
#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "Dispatcher.h"
#include "Executor.h"

namespace SimpleJSBLE {

// Native objects with an active scan or an open connection. Like a listening
// server or a socket, each keeps Node running and is itself kept alive until it
// ends. `lifecycle` says whether its objects are released on the lifecycle strand.
struct ActiveSet {
    explicit ActiveSet(bool lifecycle) : lifecycle(lifecycle) {}

    std::map<const void*, std::shared_ptr<void>> objects;
    const bool lifecycle;
};

// Per-environment state, stored as Node-API instance data.
struct AddonData {
    std::shared_ptr<Dispatcher> dispatcher;

    // Provided by the TypeScript layer through setup().
    Napi::FunctionReference make_error;  // (code, message, nativeCode?) => SimpleBLEError
    Napi::FunctionReference dispatch;    // (targetKey, event, peripheralKey?, peripheral?) => void
    Napi::FunctionReference deliver;     // (subscriptionId, value) => void

    Napi::FunctionReference backend_constructor;
    Napi::FunctionReference adapter_constructor;
    Napi::FunctionReference peripheral_constructor;

    ActiveSet scanning{true};
    ActiveSet connected{false};
};

AddonData& addon(Napi::Env env);

// A SimpleBLE failure, captured on whichever thread it happened.
struct NativeError {
    SimpleBLE::ErrorCode code;
    std::string message;
    std::optional<int32_t> native_code;
};

NativeError capture(std::exception_ptr exception) noexcept;
Napi::Value to_js(Napi::Env env, const NativeError& error);

// Runs a synchronous native call on the JavaScript thread, converting SimpleBLE
// exceptions into SimpleBLEError.
template <typename Function>
Napi::Value guard(Napi::Env env, Function&& function) {
    try {
        return function();
    } catch (const Napi::Error&) {
        throw;
    } catch (...) {
        throw Napi::Error(env, to_js(env, capture(std::current_exception())));
    }
}

// Builds the JavaScript result on the JavaScript thread, after `work` returns.
using Finisher = std::function<Napi::Value(Napi::Env)>;

Finisher finish_undefined();
Finisher finish_bool(bool value);

// Returns a promise for `work`, which runs on the executor (on `strand`, when
// given) and returns the finisher that produces the resolved value. Node stays
// alive until the promise settles.
Napi::Promise run(Napi::Env env, std::shared_ptr<Executor::Strand> strand, std::function<Finisher()> work);

// Operations on one native object run in call order. Returns the strand shared
// by every handle of the native object at `pointer`.
std::shared_ptr<Executor::Strand> strand_for(const void* pointer);

// The address of a native object is the identity the TypeScript layer uses to
// return one JavaScript object per native object.
template <typename Public>
struct Internal : Public {
    static const auto& pointer(const Public& value) { return value.*(&Internal::internal_); }
    static const void* of(const Public& value) { return pointer(value).get(); }
};

std::string key_of(const void* pointer);

// Calls the TypeScript dispatcher, which emits `event` on the object with `key`.
void emit(Napi::Env env, const std::string& key, const char* event);
void emit(Napi::Env env, const std::string& key, const char* event, const SimpleBLE::Peripheral& peripheral);

// Enumerations of backends and adapters, and releases of backends and
// adapters, run in order on this strand. Destroying a Dongl adapter closes its
// port, which an enumeration that follows must not race, or it finds the port
// still in use.
std::shared_ptr<Executor::Strand> lifecycle();

// JavaScript thread only. Records that `object` became active or ended. Doing
// either twice is a no-op, since some backends report a change both from the
// call and from a callback.
void hold_active(Napi::Env env, ActiveSet& active, const void* pointer, std::shared_ptr<void> object);
void release_active(Napi::Env env, ActiveSet& active, const void* pointer);

// Drops `value` on the executor instead of the calling thread: destroying the
// last reference to a native object can block (on Windows it calls into WinRT).
template <typename T>
void release_later(T&& value) noexcept {
    try {
        Executor::get().post([value = std::forward<T>(value)]() {});
    } catch (...) {
        // NOTE: If posting fails, the value is dropped here instead.
    }
}

// Like release_later(), for backends and adapters, on the lifecycle strand.
template <typename T>
void release_in_lifecycle(T&& value) noexcept {
    try {
        Executor::get().post(lifecycle(), [value = std::forward<T>(value)]() {});
    } catch (...) {
        // NOTE: If posting fails, the value is dropped here instead.
    }
}

Napi::Buffer<uint8_t> to_buffer(Napi::Env env, const SimpleBLE::ByteArray& bytes);
SimpleBLE::ByteArray to_bytes(const Napi::Value& value);

}  // namespace SimpleJSBLE
