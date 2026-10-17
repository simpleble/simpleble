#pragma once

#include <NitroModules/ArrayBuffer.hpp>
#include <NitroModules/Promise.hpp>
#include <simpleble/Backend.h>
#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <vector>

#include <exception>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "Executor.hpp"

namespace margelo::nitro::simplernble {

// The internal object behind a public SimpleBLE handle. Handles to the same
// native object share it, so its address identifies the object.
template <typename Public>
struct Internal : Public {
    static const void* of(const Public& value) { return (value.*(&Internal::internal_)).get(); }
};

// The exception as a JavaScript error whose message starts with SimpleBLE's
// error code, such as "[OPERATION_FAILED] ...". The TypeScript layer turns it
// into a SimpleBLEError.
std::exception_ptr to_error(std::exception_ptr exception) noexcept;

// Calls `fn` and rethrows a failure as to_error() does.
template <typename Fn>
auto guard(Fn&& fn) -> decltype(fn()) {
    try {
        return fn();
    } catch (...) {
        std::rethrow_exception(to_error(std::current_exception()));
    }
}

// Enumerations of backends and adapters run in order on this strand.
const std::shared_ptr<Executor::Strand>& lifecycle();

// Copies of bytes between SimpleBLE and JavaScript. An ArrayBuffer from
// JavaScript is only valid on the JavaScript thread, so it is copied there.
inline std::shared_ptr<ArrayBuffer> to_array_buffer(const SimpleBLE::ByteArray& value) {
    return ArrayBuffer::copy(std::vector<uint8_t>(value.data(), value.data() + value.size()));
}

inline SimpleBLE::ByteArray to_bytes(const std::shared_ptr<ArrayBuffer>& buffer) {
    if (buffer == nullptr || buffer->size() == 0) return {};
    return SimpleBLE::ByteArray(buffer->data(), buffer->size());
}

// Runs `fn` on `strand` and settles the promise with its result.
template <typename T, typename Fn>
std::shared_ptr<Promise<T>> run(const std::shared_ptr<Executor::Strand>& strand, Fn&& fn) {
    auto promise = Promise<T>::create();
    Executor::get().post(strand, [promise, fn = std::forward<Fn>(fn)]() mutable {
        try {
            if constexpr (std::is_void_v<T>) {
                fn();
                promise->resolve();
            } else {
                promise->resolve(fn());
            }
        } catch (...) {
            promise->reject(to_error(std::current_exception()));
        }
    });
    return promise;
}

// The Hybrid Object of a native object. There is one per native object while
// it is alive, so JavaScript keeps getting the same object for it. `args` go to
// the constructor of a new one.
template <typename Hybrid, typename Native, typename... Args>
std::shared_ptr<Hybrid> intern(const Native& object, Args&&... args) {
    static std::mutex mutex;
    static std::unordered_map<const void*, std::weak_ptr<Hybrid>> objects;

    const void* key = Internal<Native>::of(object);
    std::lock_guard<std::mutex> lock(mutex);
    if (auto existing = objects[key].lock()) return existing;

    // NOTE: Entries of objects JavaScript has collected are dropped as the map grows.
    if (objects.size() > 64 && objects.size() % 64 == 0) {
        for (auto it = objects.begin(); it != objects.end();) {
            it = it->second.expired() && it->first != key ? objects.erase(it) : std::next(it);
        }
    }

    auto created = std::make_shared<Hybrid>(object, std::forward<Args>(args)...);
    objects[key] = created;
    return created;
}

}  // namespace margelo::nitro::simplernble
