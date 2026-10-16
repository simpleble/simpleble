#pragma once

#include <napi.h>
#include <simpleble/Backend.h>
#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <exception>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "Dispatcher.h"
#include "Executor.h"

namespace SimpleJSBLE {

// The address of a native object is the identity the TypeScript layer uses to
// return one JavaScript object per native object.
template <typename Public>
struct Internal : Public {
    static const auto& pointer(const Public& value) { return value.*(&Internal::internal_); }
    static const void* of(const Public& value) { return pointer(value).get(); }

    // The public object for a native one, e.g. one a callback holds weakly.
    template <typename Pointer>
    static Public from(Pointer pointer) {
        Internal value;
        value.internal_ = std::move(pointer);
        return value;
    }
};

std::string key_of(const void* pointer);

// Enumerations of backends and adapters, and every release of a native object,
// run in order on this strand. A Dongl's port closes when the last object that
// uses it goes, which an enumeration that follows must not race, or it finds the
// port still in use.
std::shared_ptr<Executor::Strand> lifecycle();

// Empties `value` and drops what it held on the lifecycle strand instead of the
// calling thread: destroying the last reference to a native object can block
// (on Windows it calls into WinRT), and must come before later enumerations.
// NOTE: SimpleBLE's objects have no move constructor, so moving one copies it.
// Emptying `value` before posting keeps this thread from holding the last
// reference when the strand drops its copy.
template <typename T>
void release_later(T& value) noexcept {
    try {
        auto held = std::make_shared<T>(std::move(value));
        value = T();
        Executor::get().post(lifecycle(), [held = std::move(held)]() {});
    } catch (...) {
        // NOTE: If posting fails, the value is dropped here instead.
        value = T();
    }
}

// Native objects with an active scan or an open connection, with the adapter
// each belongs to. Like a listening server or a socket, each keeps Node running
// and is itself kept alive until it ends or its adapter is closed.
template <typename T>
struct ActiveSet {
    struct Entry {
        T object;
        const void* owner;
    };

    std::map<const void*, Entry> objects;
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

    ActiveSet<SimpleBLE::Adapter> scanning;
    ActiveSet<SimpleBLE::Peripheral> connected;
};

AddonData& addon(Napi::Env env);

// JavaScript thread only. Records that `object` became active or ended. Doing
// either twice is a no-op, since some backends report a change both from the
// call and from a callback.
template <typename T>
void hold_active(Napi::Env env, ActiveSet<T>& active, const T& object, const void* owner) {
    const void* pointer = Internal<T>::of(object);
    if (pointer == nullptr) return;
    if (!active.objects.emplace(pointer, typename ActiveSet<T>::Entry{object, owner}).second) return;
    addon(env).dispatcher->hold(env);
}

template <typename T>
void release_active(Napi::Env env, ActiveSet<T>& active, const void* pointer) {
    auto node = active.objects.extract(pointer);
    if (node.empty()) return;
    release_later(node.mapped().object);
    addon(env).dispatcher->release(env);
}

// Ends every entry of the adapter at `owner`, which is closing.
template <typename T>
void release_owned(Napi::Env env, ActiveSet<T>& active, const void* owner) {
    for (auto it = active.objects.begin(); it != active.objects.end();) {
        auto entry = it++;
        if (entry->second.owner == owner) release_active(env, active, entry->first);
    }
}

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

// A peripheral and the adapter it came from, as passed to PeripheralHandle.
struct PeripheralSource {
    SimpleBLE::Peripheral peripheral;
    const void* owner;
};

// Calls the TypeScript dispatcher, which emits `event` on the object with `key`.
void emit(Napi::Env env, const std::string& key, const char* event);

// Like emit(), with a peripheral of the adapter at `owner`. The TypeScript side
// reuses its object for the peripheral's key, and turns the External into a
// handle (peripheralHandle()) only when it has none, so frequent scan updates
// don't create handles. `peripheral` is emptied, and the External holds it only
// during the call, so nothing keeps a closed adapter's peripherals until
// garbage collection.
void emit(Napi::Env env, const std::string& key, const char* event, SimpleBLE::Peripheral& peripheral,
          const void* owner);

Napi::Buffer<uint8_t> to_buffer(Napi::Env env, const SimpleBLE::ByteArray& bytes);
SimpleBLE::ByteArray to_bytes(const Napi::Value& value);

}  // namespace SimpleJSBLE
