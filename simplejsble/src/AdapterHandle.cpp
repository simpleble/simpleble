#include "Handles.h"

#include <cstdint>

namespace SimpleJSBLE {

namespace {

// NOTE: Some backends report a scan starting or stopping only after the call
// returns, so both the event and the call record it; the second is a no-op.
// Powering off ends a scan without a scan-stop callback on several backends, so
// it counts as a stop too. An unplugged Dongl reports nothing at the adapter
// level, so its scan keeps Node running until scanStop() is called.
void scan_started(Napi::Env env, const void* pointer, const std::shared_ptr<void>& adapter) {
    auto& data = addon(env);
    if (adapter && data.scanning.emplace(pointer, adapter).second) data.dispatcher->hold(env);
}

void scan_stopped(Napi::Env env, const void* pointer) {
    auto& data = addon(env);
    if (data.scanning.erase(pointer) > 0) data.dispatcher->release(env);
}

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
                           InstanceMethod<&AdapterHandle::subscribe>("subscribe"),
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

AdapterHandle::~AdapterHandle() { release_later(std::move(adapter_)); }

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
        return [adapter](Napi::Env env) {
            scan_started(env, Internal<SimpleBLE::Adapter>::of(adapter),
                         Internal<SimpleBLE::Adapter>::pointer(adapter));
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
    if (!(timeout_ms >= 0 && timeout_ms <= INT32_MAX) || timeout_ms != static_cast<int32_t>(timeout_ms)) {
        throw Napi::RangeError::New(info.Env(), "timeoutMs must be an integer from 0 to 2147483647");
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
    return guard(info.Env(), [&] { return wrap_peripherals(info.Env(), adapter_.scan_get_results()); });
}

// NOTE: Unlike SimplePyBLE's aio module, these two return promises: on Windows
// they run a device query through WinRT.
Napi::Value AdapterHandle::get_paired_peripherals(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter]() mutable -> Finisher {
        auto peripherals = adapter.get_paired_peripherals();
        return [peripherals](Napi::Env env) { return wrap_peripherals(env, peripherals); };
    });
}

Napi::Value AdapterHandle::get_connected_peripherals(const Napi::CallbackInfo& info) {
    auto adapter = adapter_;
    return run(info.Env(), strand(), [adapter]() mutable -> Finisher {
        auto peripherals = adapter.get_connected_peripherals();
        return [peripherals](Napi::Env env) { return wrap_peripherals(env, peripherals); };
    });
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
    std::weak_ptr<void> native = Internal<SimpleBLE::Adapter>::pointer(adapter_);

    auto on_scan_start = [events, pointer, native]() {
        auto dispatcher = events->dispatcher.lock();
        if (!dispatcher) return;
        dispatcher->post([events, pointer, native](Napi::Env env) {
            scan_started(env, pointer, native.lock());
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

        adapter_.set_callback_on_scan_found([events](SimpleBLE::Peripheral peripheral) {
            auto dispatcher = events->dispatcher.lock();
            if (!dispatcher) return;
            dispatcher->post(
                [events, peripheral](Napi::Env env) { emit_peripheral(env, events->key, "scanFound", peripheral); });
        });

        // A device can advertise many times a second. Only one update per
        // device is queued at a time; the getters always read current data.
        adapter_.set_callback_on_scan_updated([events](SimpleBLE::Peripheral peripheral) {
            auto dispatcher = events->dispatcher.lock();
            if (!dispatcher) return;
            Dispatcher::Key key{events.get(), Internal<SimpleBLE::Peripheral>::of(peripheral)};
            dispatcher->post_coalesced(key, [events, peripheral](Napi::Env env) {
                emit_peripheral(env, events->key, "scanUpdated", peripheral);
            });
        });
        return info.Env().Undefined();
    });
}

}  // namespace SimpleJSBLE
