#include "Handles.h"

namespace SimpleJSBLE {

Napi::Function BackendHandle::define(Napi::Env env) {
    return DefineClass(env, "BackendHandle",
                       {
                           InstanceMethod<&BackendHandle::key>("key"),
                           InstanceMethod<&BackendHandle::identifier>("identifier"),
                           InstanceMethod<&BackendHandle::bluetooth_enabled>("bluetoothEnabled"),
                           InstanceMethod<&BackendHandle::adapters>("adapters"),
                       });
}

Napi::Object BackendHandle::wrap(Napi::Env env, const SimpleBLE::Backend& backend) {
    SimpleBLE::Backend copy = backend;
    return addon(env).backend_constructor.New({Napi::External<SimpleBLE::Backend>::New(env, &copy)});
}

BackendHandle::BackendHandle(const Napi::CallbackInfo& info) : Napi::ObjectWrap<BackendHandle>(info) {
    if (!info[0].IsExternal()) throw Napi::TypeError::New(info.Env(), "Use Backend.getBackends()");

    backend_ = *info[0].As<Napi::External<SimpleBLE::Backend>>().Data();
}

BackendHandle::~BackendHandle() { release_later(std::move(backend_)); }

Napi::Value BackendHandle::key(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(Internal<SimpleBLE::Backend>::of(backend_)));
}

Napi::Value BackendHandle::identifier(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::String::New(info.Env(), backend_.identifier()); });
}

Napi::Value BackendHandle::bluetooth_enabled(const Napi::CallbackInfo& info) {
    auto backend = backend_;
    return run(info.Env(), lifecycle(), [backend]() mutable { return finish_bool(backend.bluetooth_enabled()); });
}

Napi::Value BackendHandle::adapters(const Napi::CallbackInfo& info) {
    auto backend = backend_;
    return run(info.Env(), lifecycle(), [backend]() mutable -> Finisher {
        auto adapters = backend.adapters();
        return [adapters](Napi::Env env) { return wrap_adapters(env, adapters); };
    });
}

}  // namespace SimpleJSBLE
