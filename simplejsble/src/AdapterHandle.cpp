#include <simpleble/Advanced.h>

#include "Handles.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SimpleJSBLE {

namespace {

// NOTE: Some backends report a scan starting or stopping only after the call
// returns, so both the event and the call record it; the second is a no-op.
// Powering off ends a scan without a scan-stop callback on several backends, so
// it counts as a stop too. An unplugged Dongl reports nothing at the adapter
// level, so its scan keeps Node running until scanStop() is called.
void scan_started(Napi::Env env, const SimpleBLE::Adapter& adapter) {
    hold_active(env, addon(env).scanning, adapter, Internal<SimpleBLE::Adapter>::of(adapter));
}

void scan_stopped(Napi::Env env, const void* pointer) { release_active(env, addon(env).scanning, pointer); }

}  // namespace

Napi::Function AdapterHandle::define(Napi::Env env) {
    return DefineClass(env, "AdapterHandle",
                       {
                           InstanceMethod<&AdapterHandle::key>("key"),
                           InstanceMethod<&AdapterHandle::identifier>("identifier"),
                           InstanceMethod<&AdapterHandle::address>("address"),
                           InstanceMethod<&AdapterHandle::is_powered>("isPowered"),
                           InstanceMethod<&AdapterHandle::power_on>("powerOn"),
                           InstanceMethod<&AdapterHandle::power_off>("powerOff"),
                           InstanceMethod<&AdapterHandle::scan_start>("scanStart"),
                           InstanceMethod<&AdapterHandle::scan_stop>("scanStop"),
                           InstanceMethod<&AdapterHandle::scan_for>("scanFor"),
                           InstanceMethod<&AdapterHandle::scan_is_active>("scanIsActive"),
                           InstanceMethod<&AdapterHandle::scan_get_results>("scanGetResults"),
                           InstanceMethod<&AdapterHandle::get_paired_peripherals>("getPairedPeripherals"),
                           InstanceMethod<&AdapterHandle::get_connected_peripherals>("getConnectedPeripherals"),
                           InstanceMethod<&AdapterHandle::retrieve_cached_peripherals>("retrieveCachedPeripherals"),
                           InstanceMethod<&AdapterHandle::subscribe>("subscribe"),
                           InstanceMethod<&AdapterHandle::create_local_peripheral>("createLocalPeripheral"),
                           InstanceMethod<&AdapterHandle::connections>("connections"),
                           InstanceMethod<&AdapterHandle::hosts>("hosts"),
                           InstanceMethod<&AdapterHandle::close>("close"),
                           InstanceMethod<&AdapterHandle::release>("release"),
                       });
}

Napi::Object AdapterHandle::wrap(Napi::Env env, const SimpleBLE::Adapter& adapter) {
    SimpleBLE::Adapter copy = adapter;
    return addon(env).adapter_constructor.New({Napi::External<SimpleBLE::Adapter>::New(env, &copy)});
}

Napi::Array wrap_adapters(Napi::Env env, const std::vector<SimpleBLE::Adapter>& adapters) {
    auto array = Napi::Array::New(env, adapters.size());
    for (uint32_t i = 0; i < adapters.size(); i++) {
        array.Set(i, AdapterHandle::wrap(env, adapters[i]));
    }
    return array;
}

AdapterHandle::AdapterHandle(const Napi::CallbackInfo& info) : Napi::ObjectWrap<AdapterHandle>(info) {
    if (!info[0].IsExternal()) throw Napi::TypeError::New(info.Env(), "Use Adapter.getAdapters()");

    adapter_ = *info[0].As<Napi::External<SimpleBLE::Adapter>>().Data();
}

AdapterHandle::~AdapterHandle() { release_later(adapter_); }

std::shared_ptr<Executor::Strand> AdapterHandle::strand() {
    if (!strand_) strand_ = strand_for(Internal<SimpleBLE::Adapter>::of(adapter_));
    return strand_;
}

Napi::Value AdapterHandle::key(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(Internal<SimpleBLE::Adapter>::of(adapter_)));
}

Napi::Value AdapterHandle::identifier(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::String::New(info.Env(), adapter_.identifier()); });
}

Napi::Value AdapterHandle::address(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::String::New(info.Env(), adapter_.address()); });
}

// NOTE: Unlike SimplePyBLE's aio module, power and scan state return promises:
// a Dongl answers them over its serial port.
Napi::Value AdapterHandle::is_powered(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter]() mutable { return finish_bool(adapter.is_powered()); });
}

Napi::Value AdapterHandle::power_on(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter]() mutable {
        adapter.power_on();
        return finish_undefined();
    });
}

Napi::Value AdapterHandle::power_off(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter]() mutable -> Finisher {
        adapter.power_off();
        return [adapter](Napi::Env env) {
            scan_stopped(env, Internal<SimpleBLE::Adapter>::of(adapter));
            return env.Undefined();
        };
    });
}

Napi::Value AdapterHandle::scan_start(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter]() mutable -> Finisher {
        adapter.scan_start();
        // NOTE: With Bluetooth off, SimpleBLE logs a warning and doesn't scan, so
        // only a scan that is active or may still report starting keeps Node running.
        const bool scanning = adapter.scan_is_active() || adapter.is_powered();
        return [adapter, scanning](Napi::Env env) {
            if (scanning) scan_started(env, adapter);
            return env.Undefined();
        };
    });
}

Napi::Value AdapterHandle::scan_stop(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter]() mutable -> Finisher {
        adapter.scan_stop();
        return [adapter](Napi::Env env) {
            scan_stopped(env, Internal<SimpleBLE::Adapter>::of(adapter));
            return env.Undefined();
        };
    });
}

Napi::Value AdapterHandle::scan_for(const Napi::CallbackInfo& info) {
    if (!info[0].IsNumber()) throw Napi::TypeError::New(info.Env(), "timeoutMs must be a number");
    const double timeout_ms = info[0].As<Napi::Number>().DoubleValue();
    if (!(timeout_ms >= INT32_MIN && timeout_ms <= INT32_MAX) || timeout_ms != static_cast<int32_t>(timeout_ms)) {
        throw Napi::RangeError::New(info.Env(), "timeoutMs must be a 32-bit integer");
    }

    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter, timeout_ms]() mutable -> Finisher {
        adapter.scan_for(static_cast<int>(timeout_ms));
        return [adapter](Napi::Env env) {
            scan_stopped(env, Internal<SimpleBLE::Adapter>::of(adapter));
            return env.Undefined();
        };
    });
}

Napi::Value AdapterHandle::scan_is_active(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter]() mutable { return finish_bool(adapter.scan_is_active()); });
}

Napi::Value AdapterHandle::scan_get_results(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] {
        return wrap_peripherals(info.Env(), adapter_.scan_get_results(), Internal<SimpleBLE::Adapter>::of(adapter_));
    });
}

// NOTE: Unlike SimplePyBLE's aio module, these two return promises: on Windows
// they run a device query through WinRT.
Napi::Value AdapterHandle::get_paired_peripherals(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    const void* owner = Internal<SimpleBLE::Adapter>::of(adapter_);
    return run(info.Env(), strand(), [adapter, owner]() mutable -> Finisher {
        auto peripherals = adapter.get_paired_peripherals();
        return [peripherals, owner](Napi::Env env) { return wrap_peripherals(env, peripherals, owner); };
    });
}

Napi::Value AdapterHandle::get_connected_peripherals(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    const void* owner = Internal<SimpleBLE::Adapter>::of(adapter_);
    return run(info.Env(), strand(), [adapter, owner]() mutable -> Finisher {
        auto peripherals = adapter.get_connected_peripherals();
        return [peripherals, owner](Napi::Env env) { return wrap_peripherals(env, peripherals, owner); };
    });
}

// NOTE: Only CoreBluetooth on macOS keeps a cache of known peripherals.
Napi::Value AdapterHandle::retrieve_cached_peripherals(const Napi::CallbackInfo& info) {
    if (!info[0].IsArray()) throw Napi::TypeError::New(info.Env(), "identifiers must be an array of strings");
    std::vector<SimpleBLE::BluetoothAddress> identifiers;
    auto array = info[0].As<Napi::Array>();
    for (uint32_t i = 0; i < array.Length(); i++) {
        Napi::Value item = array.Get(i);
        if (!item.IsString()) throw Napi::TypeError::New(info.Env(), "identifiers must be an array of strings");
        identifiers.push_back(item.As<Napi::String>().Utf8Value());
    }

#if SIMPLEBLE_BACKEND_MACOS
    auto adapter = adapter_;
    const void* owner = Internal<SimpleBLE::Adapter>::of(adapter_);
    return run(info.Env(), strand(), [adapter, owner, identifiers]() mutable -> Finisher {
        auto peripherals = SimpleBLE::Advanced::MacOS::retrieve_cached_peripherals(adapter, identifiers);
        return [peripherals, owner](Napi::Env env) { return wrap_peripherals(env, peripherals, owner); };
    });
#else
    return run(info.Env(), strand(), []() -> Finisher { throw SimpleBLE::Exception::OperationNotSupported(); });
#endif
}

// Called once, when the TypeScript Adapter is created. Events are queued even
// without listeners, so a listener added right after a call still sees the
// events that call causes; the TypeScript side drops unheard ones.
Napi::Value AdapterHandle::subscribe(const Napi::CallbackInfo& info) {
    auto events = std::make_shared<EventTarget>();
    events->dispatcher = addon(info.Env()).dispatcher;
    events->key = key_of(Internal<SimpleBLE::Adapter>::of(adapter_));

    // NOTE: Callbacks are never cleared. They stay with the native adapter,
    // which outlives this handle, and do nothing once the environment is gone.
    auto signal = [events](const char* event) {
        return [events, event]() {
            auto dispatcher = events->dispatcher.lock();
            if (!dispatcher) return;
            dispatcher->post([events, event](Napi::Env env) { emit(env, events->key, event); });
        };
    };

    // NOTE: The callbacks are stored in the native adapter, so they hold it weakly.
    const void* pointer = Internal<SimpleBLE::Adapter>::of(adapter_);
    std::weak_ptr native = Internal<SimpleBLE::Adapter>::pointer(adapter_);

    auto on_scan_start = [events, native]() {
        auto dispatcher = events->dispatcher.lock();
        if (!dispatcher) return;
        dispatcher->post([events, native](Napi::Env env) {
            auto adapter = Internal<SimpleBLE::Adapter>::from(native.lock());
            scan_started(env, adapter);
            release_later(adapter);
            emit(env, events->key, "scanStart");
        });
    };

    auto stop_signal = [events, pointer](const char* event) {
        return [events, pointer, event]() {
            auto dispatcher = events->dispatcher.lock();
            if (!dispatcher) return;
            dispatcher->post([events, pointer, event](Napi::Env env) {
                scan_stopped(env, pointer);
                emit(env, events->key, event);
            });
        };
    };

    return guard(info.Env(), [&] {
        adapter_.set_callback_on_scan_start(on_scan_start);
        adapter_.set_callback_on_scan_stop(stop_signal("scanStop"));
        adapter_.set_callback_on_power_on(signal("powerOn"));
        adapter_.set_callback_on_power_off(stop_signal("powerOff"));

        adapter_.set_callback_on_scan_found([events, pointer](SimpleBLE::Peripheral peripheral) {
            auto dispatcher = events->dispatcher.lock();
            if (!dispatcher) return;
            dispatcher->post([events, pointer, peripheral](Napi::Env env) mutable {
                emit(env, events->key, "scanFound", peripheral, pointer);
            });
        });

        // A device can advertise many times a second. Only one update per
        // device is queued at a time; the getters always read current data.
        adapter_.set_callback_on_scan_updated([events, pointer](SimpleBLE::Peripheral peripheral) {
            auto dispatcher = events->dispatcher.lock();
            if (!dispatcher) return;
            Dispatcher::Key key{events.get(), Internal<SimpleBLE::Peripheral>::of(peripheral)};
            dispatcher->post_coalesced(key, [events, pointer, peripheral](Napi::Env env) mutable {
                emit(env, events->key, "scanUpdated", peripheral, pointer);
            });
        });
        return info.Env().Undefined();
    });
}

// The peripherals connected through this adapter, which close() disconnects
// before releasing them. Unlike getConnectedPeripherals(), this leaves out
// connections other programs made.
Napi::Value AdapterHandle::connections(const Napi::CallbackInfo& info) {
    const void* pointer = Internal<SimpleBLE::Adapter>::of(adapter_);
    std::vector<SimpleBLE::Peripheral> peripherals;
    for (const auto& [key, entry] : addon(info.Env()).connected.objects) {
        if (entry.owner == pointer) peripherals.push_back(entry.object);
    }
    return wrap_peripherals(info.Env(), peripherals, pointer);
}

Napi::Value AdapterHandle::create_local_peripheral(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] {
        return LocalPeripheralHandle::wrap(info.Env(), adapter_.create_local_peripheral(),
                                           Internal<SimpleBLE::Adapter>::of(adapter_));
    });
}

// The started local peripherals of this adapter, which close() stops.
Napi::Value AdapterHandle::hosts(const Napi::CallbackInfo& info) {
    const void* pointer = Internal<SimpleBLE::Adapter>::of(adapter_);
    auto array = Napi::Array::New(info.Env());
    uint32_t count = 0;
    for (const auto& [key, entry] : addon(info.Env()).hosting.objects) {
        if (entry.owner == pointer) array.Set(count++, LocalPeripheralHandle::wrap(info.Env(), entry.object, pointer));
    }
    return array;
}

// Releases everything the addon holds of this adapter, after the TypeScript
// side has stopped its scan and local peripherals, disconnected its peripherals
// and released their handles. The promise settles once the lifecycle strand has dropped them, so
// an enumeration that follows can open the adapter again.
Napi::Value AdapterHandle::close(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    const void* pointer = Internal<SimpleBLE::Adapter>::of(adapter_);
    release_owned(env, addon(env).scanning, pointer);
    release_owned(env, addon(env).connected, pointer);
    release_owned(env, addon(env).hosting, pointer);
    release_later(adapter_);
    return run(env, lifecycle(), [] { return finish_undefined(); });
}

// Drops the adapter of a handle the TypeScript side doesn't use.
Napi::Value AdapterHandle::release(const Napi::CallbackInfo& info) {
    release_later(adapter_);
    return info.Env().Undefined();
}

}  // namespace SimpleJSBLE
