#include "Addon.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <mutex>
#include <new>

namespace SimpleJSBLE {

AddonData& addon(Napi::Env env) { return *env.GetInstanceData<AddonData>(); }

NativeError capture(std::exception_ptr exception) noexcept {
    try {
        std::rethrow_exception(exception);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        std::optional<int32_t> native_code;
        if (auto winrt = dynamic_cast<const SimpleBLE::Exception::WinRTException*>(&e)) native_code = winrt->code();

        try {
            return {e.make_error()->code, e.what(), native_code};
        } catch (...) {
            return {SimpleBLE::ErrorCode::OUT_OF_MEMORY, e.what(), native_code};
        }
    } catch (const std::bad_alloc& e) {
        return {SimpleBLE::ErrorCode::OUT_OF_MEMORY, e.what(), std::nullopt};
    } catch (const std::exception& e) {
        return {SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, e.what(), std::nullopt};
    } catch (...) {
        return {SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown native exception", std::nullopt};
    }
}

namespace {

const char* code_name(SimpleBLE::ErrorCode code) {
    switch (code) {
        case SimpleBLE::ErrorCode::INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";
        case SimpleBLE::ErrorCode::OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case SimpleBLE::ErrorCode::OBJECT_NOT_INITIALIZED:
            return "OBJECT_NOT_INITIALIZED";
        case SimpleBLE::ErrorCode::INVALID_BACKEND_REFERENCE:
            return "INVALID_BACKEND_REFERENCE";
        case SimpleBLE::ErrorCode::PERIPHERAL_NOT_CONNECTED:
            return "PERIPHERAL_NOT_CONNECTED";
        case SimpleBLE::ErrorCode::GATT_SERVICE_NOT_FOUND:
            return "GATT_SERVICE_NOT_FOUND";
        case SimpleBLE::ErrorCode::GATT_CHARACTERISTIC_NOT_FOUND:
            return "GATT_CHARACTERISTIC_NOT_FOUND";
        case SimpleBLE::ErrorCode::GATT_DESCRIPTOR_NOT_FOUND:
            return "GATT_DESCRIPTOR_NOT_FOUND";
        case SimpleBLE::ErrorCode::OPERATION_NOT_SUPPORTED:
            return "OPERATION_NOT_SUPPORTED";
        case SimpleBLE::ErrorCode::OPERATION_FAILED:
            return "OPERATION_FAILED";
        case SimpleBLE::ErrorCode::WINRT_ACCESS_DENIED:
            return "WINRT_ACCESS_DENIED";
        case SimpleBLE::ErrorCode::WINRT_EXCEPTION:
            return "WINRT_EXCEPTION";
        case SimpleBLE::ErrorCode::CORE_BLUETOOTH_EXCEPTION:
            return "CORE_BLUETOOTH_EXCEPTION";
        case SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION:
            return "UNCLASSIFIED_EXCEPTION";
        case SimpleBLE::ErrorCode::ADAPTER_IN_USE:
            return "ADAPTER_IN_USE";
    }
    return "UNCLASSIFIED_EXCEPTION";
}

}  // namespace

Napi::Value to_js(Napi::Env env, const NativeError& error) {
    Napi::Value native_code = env.Undefined();
    if (error.native_code) native_code = Napi::Number::New(env, *error.native_code);

    return addon(env).make_error.Call(
        {Napi::String::New(env, code_name(error.code)), Napi::String::New(env, error.message), native_code});
}

Finisher finish_undefined() {
    return [](Napi::Env env) { return env.Undefined(); };
}

Finisher finish_bool(bool value) {
    return [value](Napi::Env env) { return Napi::Boolean::New(env, value); };
}

Napi::Promise run(Napi::Env env, std::shared_ptr<Executor::Strand> strand, std::function<Finisher()> work) {
    auto dispatcher = addon(env).dispatcher;
    auto deferred = std::make_shared<Napi::Promise::Deferred>(env);
    auto promise = deferred->Promise();

    auto task = [dispatcher, deferred, work = std::move(work)]() mutable {
        Finisher finisher;
        std::optional<NativeError> error;
        try {
            finisher = work();
        } catch (...) {
            error = capture(std::current_exception());
        }
        work = nullptr;

        dispatcher->post([dispatcher, deferred, finisher = std::move(finisher), error](Napi::Env env) mutable {
            dispatcher->release(env);
            if (error) {
                deferred->Reject(to_js(env, *error));
                return;
            }
            try {
                deferred->Resolve(finisher(env));
            } catch (const Napi::Error& e) {
                deferred->Reject(e.Value());
            } catch (...) {
                deferred->Reject(to_js(env, capture(std::current_exception())));
            }
            // The finisher can hold the last reference to a native object.
            release_later(std::move(finisher));
        });
    };

    if (strand) {
        Executor::get().post(strand, std::move(task));
    } else {
        Executor::get().post(std::move(task));
    }
    // NOTE: The completion runs on this thread, so it can't release before this.
    dispatcher->hold(env);
    return promise;
}

std::shared_ptr<Executor::Strand> lifecycle() {
    // NOTE: Never destroyed, like the executor.
    static auto strand = new std::shared_ptr<Executor::Strand>(std::make_shared<Executor::Strand>());
    return *strand;
}

std::shared_ptr<Executor::Strand> strand_for(const void* pointer) {
    static std::mutex mutex;
    static std::map<const void*, std::weak_ptr<Executor::Strand>> strands;
    static std::size_t sweep_at = 64;

    std::lock_guard<std::mutex> lock(mutex);
    auto& slot = strands[pointer];
    auto strand = slot.lock();
    if (strand) return strand;

    strand = std::make_shared<Executor::Strand>();
    slot = strand;

    // Forget the strands of native objects that are gone, now and then.
    if (strands.size() >= sweep_at) {
        for (auto it = strands.begin(); it != strands.end();) {
            if (it->second.expired()) {
                it = strands.erase(it);
            } else {
                ++it;
            }
        }
        sweep_at = std::max<std::size_t>(64, 2 * strands.size());
    }
    return strand;
}

std::string key_of(const void* pointer) {
    char buffer[2 + 2 * sizeof(void*) + 1];
    std::snprintf(buffer, sizeof(buffer), "%p", pointer);
    return buffer;
}

void emit(Napi::Env env, const std::string& key, const char* event) {
    addon(env).dispatch.Call({Napi::String::New(env, key), Napi::String::New(env, event)});
}

void emit(Napi::Env env, const std::string& key, const char* event, const std::string& peripheral_key) {
    addon(env).dispatch.Call(
        {Napi::String::New(env, key), Napi::String::New(env, event), Napi::String::New(env, peripheral_key)});
}

Napi::Buffer<uint8_t> to_buffer(Napi::Env env, const SimpleBLE::ByteArray& bytes) {
    return Napi::Buffer<uint8_t>::Copy(env, bytes.data(), bytes.size());
}

SimpleBLE::ByteArray to_bytes(const Napi::Value& value) {
    if (!value.IsTypedArray()) throw Napi::TypeError::New(value.Env(), "Expected a Buffer or Uint8Array");

    auto array = value.As<Napi::TypedArray>();
    if (array.TypedArrayType() != napi_uint8_array) {
        throw Napi::TypeError::New(value.Env(), "Expected a Buffer or Uint8Array");
    }
    auto bytes = array.As<Napi::Uint8Array>();
    return SimpleBLE::ByteArray(bytes.Data(), bytes.ElementLength());
}

}  // namespace SimpleJSBLE
