#include <napi.h>
#include <simpleble/Backend.h>
#include <simpleble/SimpleBLE.h>

#include <atomic>
#include <chrono>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "Addon.h"
#include "Handles.h"

namespace SimpleJSBLE {
namespace {

// NOTE: Node unloads an addon when the last environment that loaded it exits,
// such as a worker that used it alone. The executor's threads and SimpleBLE's
// own (D-Bus, Dongl serial, WinRT) would then run code that is no longer mapped,
// so the library pins itself and stays loaded for the life of the process.
void pin_library() {
#ifdef _WIN32
    HMODULE module;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                       reinterpret_cast<LPCWSTR>(&pin_library), &module);
#else
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&pin_library), &info) != 0 && info.dli_fname != nullptr) {
        dlopen(info.dli_fname, RTLD_LAZY | RTLD_NODELETE | RTLD_NOLOAD);
    }
#endif
}

// NOTE: SimpleBLE's backends are process-wide and each native object has a
// single slot per callback. Two Node environments (e.g. the main thread and a
// worker) would take each other's callbacks, so one environment owns the addon
// at a time. Loading it again in the same environment is fine.
std::atomic<napi_env> owner{nullptr};

Napi::Value setup(const Napi::CallbackInfo& info) {
    auto options = info[0].As<Napi::Object>();
    auto& data = addon(info.Env());
    data.make_error = Napi::Persistent(options.Get("makeError").As<Napi::Function>());
    data.dispatch = Napi::Persistent(options.Get("dispatch").As<Napi::Function>());
    return info.Env().Undefined();
}

Napi::Value get_backends(const Napi::CallbackInfo& info) {
    return run(info.Env(), lifecycle(), []() -> Finisher {
        auto backends = SimpleBLE::Backend::get_backends();
        return [backends](Napi::Env env) {
            auto array = Napi::Array::New(env, backends.size());
            for (uint32_t i = 0; i < backends.size(); i++) {
                array.Set(i, BackendHandle::wrap(env, backends[i]));
            }
            return array;
        };
    });
}

template <SimpleBLE::Backend (*Get)()>
Napi::Value get_backend(const Napi::CallbackInfo& info) {
    return run(info.Env(), lifecycle(), []() -> Finisher {
        auto backend = Get();
        return [backend](Napi::Env env) { return BackendHandle::wrap(env, backend); };
    });
}

Napi::Value get_adapters(const Napi::CallbackInfo& info) {
    return run(info.Env(), lifecycle(), []() -> Finisher {
        auto adapters = SimpleBLE::Adapter::get_adapters();
        return [adapters](Napi::Env env) { return wrap_adapters(env, adapters); };
    });
}

Napi::Value bluetooth_enabled(const Napi::CallbackInfo& info) {
    return run(info.Env(), lifecycle(), [] { return finish_bool(SimpleBLE::Adapter::bluetooth_enabled()); });
}

// Configuration. Durations are exchanged in milliseconds.

namespace Config = SimpleBLE::Config;

bool* bool_setting(const std::string& name) {
    if (name == "simpleBluez.useSystemBus") return &Config::SimpleBluez::use_system_bus;
    if (name == "winRT.experimentalUseOwnMtaApartment") return &Config::WinRT::experimental_use_own_mta_apartment;
    if (name == "winRT.experimentalReinitializeWinrtApartmentOnMainThread") {
        return &Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread;
    }
    if (name == "winRT.useDeferredDisconnect") return &Config::WinRT::use_deferred_disconnect;
    if (name == "dongl.useDonglBackend") return &Config::Dongl::use_dongl_backend;
    if (name == "dongl.autoUpdate") return &Config::Dongl::auto_update;
    if (name == "dongl.forceUpdate") return &Config::Dongl::force_update;
    return nullptr;
}

std::chrono::steady_clock::duration* duration_setting(const std::string& name) {
    if (name == "simpleBluez.connectionTimeout") return &Config::SimpleBluez::connection_timeout;
    if (name == "simpleBluez.disconnectionTimeout") return &Config::SimpleBluez::disconnection_timeout;
    return nullptr;
}

Napi::Value config_get(const Napi::CallbackInfo& info) {
    const auto name = info[0].As<Napi::String>().Utf8Value();
    if (auto setting = bool_setting(name)) return Napi::Boolean::New(info.Env(), *setting);
    if (auto setting = duration_setting(name)) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(*setting).count();
        return Napi::Number::New(info.Env(), static_cast<double>(ms));
    }
    throw Napi::Error::New(info.Env(), "Unknown setting: " + name);
}

Napi::Value config_set(const Napi::CallbackInfo& info) {
    const auto name = info[0].As<Napi::String>().Utf8Value();
    if (auto setting = bool_setting(name)) {
        if (!info[1].IsBoolean()) throw Napi::TypeError::New(info.Env(), name + " must be a boolean");
        *setting = info[1].As<Napi::Boolean>().Value();
        return info.Env().Undefined();
    }
    if (auto setting = duration_setting(name)) {
        if (!info[1].IsNumber()) throw Napi::TypeError::New(info.Env(), name + " must be a number of milliseconds");
        const double ms = info[1].As<Napi::Number>().DoubleValue();
        if (!(ms >= 0)) throw Napi::RangeError::New(info.Env(), name + " cannot be negative");
        *setting = std::chrono::milliseconds(static_cast<int64_t>(ms));
        return info.Env().Undefined();
    }
    throw Napi::Error::New(info.Env(), "Unknown setting: " + name);
}

Napi::Value config_reset(const Napi::CallbackInfo& info) {
    const auto group = info[0].As<Napi::String>().Utf8Value();
    if (group == "simpleBluez") {
        Config::SimpleBluez::reset();
    } else if (group == "winRT") {
        Config::WinRT::reset();
    } else if (group == "dongl") {
        Config::Dongl::reset();
    } else if (group == "all") {
        Config::Base::reset_all();
    } else {
        throw Napi::Error::New(info.Env(), "Unknown setting group: " + group);
    }
    return info.Env().Undefined();
}

void finalize(Napi::Env, AddonData* data) {
    data->dispatcher->close();
    release_later(std::move(data->scanning));
    delete data;
    owner = nullptr;
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
    pin_library();

    napi_env expected = nullptr;
    if (!owner.compare_exchange_strong(expected, static_cast<napi_env>(env)) && expected != env) {
        auto error = Napi::Error::New(env, "simplejsble is already loaded in another Node environment of this process");
        error.Set("code", "ERR_SIMPLEJSBLE_ENVIRONMENT");
        throw error;
    }

    auto data = env.GetInstanceData<AddonData>();
    if (data == nullptr) {
        data = new AddonData();
        data->dispatcher = std::make_shared<Dispatcher>(env);
        env.SetInstanceData<AddonData, finalize>(data);
    }

    data->backend_constructor = Napi::Persistent(BackendHandle::define(env));
    data->adapter_constructor = Napi::Persistent(AdapterHandle::define(env));
    data->peripheral_constructor = Napi::Persistent(PeripheralHandle::define(env));

    exports.Set("setup", Napi::Function::New<setup>(env, "setup"));
    exports.Set("getBackends", Napi::Function::New<get_backends>(env, "getBackends"));
    exports.Set("getNativeBackend", Napi::Function::New<get_backend<&SimpleBLE::Backend::get_native>>(env));
    exports.Set("getDonglBackend", Napi::Function::New<get_backend<&SimpleBLE::Backend::get_dongl>>(env));
    exports.Set("getSimulatorBackend", Napi::Function::New<get_backend<&SimpleBLE::Backend::get_simulator>>(env));
    exports.Set("getAdapters", Napi::Function::New<get_adapters>(env, "getAdapters"));
    exports.Set("eventPeripheral", Napi::Function::New<event_peripheral>(env, "eventPeripheral"));
    exports.Set("bluetoothEnabled", Napi::Function::New<bluetooth_enabled>(env, "bluetoothEnabled"));
    exports.Set("configGet", Napi::Function::New<config_get>(env, "configGet"));
    exports.Set("configSet", Napi::Function::New<config_set>(env, "configSet"));
    exports.Set("configReset", Napi::Function::New<config_reset>(env, "configReset"));
    return exports;
}

}  // namespace
}  // namespace SimpleJSBLE

// NOTE: NODE_API_MODULE pastes the name into an identifier, so it can't be qualified.
static Napi::Object Init(Napi::Env env, Napi::Object exports) { return SimpleJSBLE::init(env, exports); }

NODE_API_MODULE(simplejsble, Init)
