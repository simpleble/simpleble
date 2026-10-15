#include "Handles.h"

#include <cstdint>
#include <limits>

namespace SimpleJSBLE {

Napi::Function PeripheralHandle::define(Napi::Env env) {
    return DefineClass(env, "PeripheralHandle",
                       {
                           InstanceMethod<&PeripheralHandle::key>("key"),
                           InstanceMethod<&PeripheralHandle::identifier>("identifier"),
                           InstanceMethod<&PeripheralHandle::address>("address"),
                           InstanceMethod<&PeripheralHandle::address_type>("addressType"),
                           InstanceMethod<&PeripheralHandle::rssi>("rssi"),
                           InstanceMethod<&PeripheralHandle::tx_power>("txPower"),
                           InstanceMethod<&PeripheralHandle::mtu>("mtu"),
                           InstanceMethod<&PeripheralHandle::is_connected>("isConnected"),
                           InstanceMethod<&PeripheralHandle::is_connectable>("isConnectable"),
                           InstanceMethod<&PeripheralHandle::is_paired>("isPaired"),
                           InstanceMethod<&PeripheralHandle::manufacturer_data>("manufacturerData"),
                       });
}

Napi::Object PeripheralHandle::wrap(Napi::Env env, const SimpleBLE::Peripheral& peripheral) {
    SimpleBLE::Peripheral copy = peripheral;
    return addon(env).peripheral_constructor.New({Napi::External<SimpleBLE::Peripheral>::New(env, &copy)});
}

Napi::Array wrap_peripherals(Napi::Env env, const std::vector<SimpleBLE::Peripheral>& peripherals) {
    auto array = Napi::Array::New(env, peripherals.size());
    for (uint32_t i = 0; i < peripherals.size(); i++) {
        array.Set(i, PeripheralHandle::wrap(env, peripherals[i]));
    }
    return array;
}

PeripheralHandle::PeripheralHandle(const Napi::CallbackInfo& info) : Napi::ObjectWrap<PeripheralHandle>(info) {
    if (!info[0].IsExternal()) throw Napi::TypeError::New(info.Env(), "Peripherals come from an Adapter");

    peripheral_ = *info[0].As<Napi::External<SimpleBLE::Peripheral>>().Data();
}

PeripheralHandle::~PeripheralHandle() { release_later(std::move(peripheral_)); }

std::shared_ptr<Executor::Strand> PeripheralHandle::strand() {
    if (!strand_) strand_ = strand_for(Internal<SimpleBLE::Peripheral>::of(peripheral_));
    return strand_;
}

void emit_peripheral(Napi::Env env, const std::string& key, const char* event,
                     const SimpleBLE::Peripheral& peripheral) {
    auto& data = addon(env);
    data.event_peripheral = peripheral;
    try {
        emit(env, key, event, key_of(Internal<SimpleBLE::Peripheral>::of(peripheral)));
    } catch (...) {
        data.event_peripheral.reset();
        throw;
    }
    data.event_peripheral.reset();
}

Napi::Value event_peripheral(const Napi::CallbackInfo& info) {
    auto& data = addon(info.Env());
    if (!data.event_peripheral) throw Napi::Error::New(info.Env(), "No peripheral event is being dispatched");
    return PeripheralHandle::wrap(info.Env(), *data.event_peripheral);
}

Napi::Value PeripheralHandle::key(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(Internal<SimpleBLE::Peripheral>::of(peripheral_)));
}

Napi::Value PeripheralHandle::identifier(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::String::New(info.Env(), peripheral_.identifier()); });
}

Napi::Value PeripheralHandle::address(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::String::New(info.Env(), peripheral_.address()); });
}

Napi::Value PeripheralHandle::address_type(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] {
        switch (peripheral_.address_type()) {
            case SimpleBLE::BluetoothAddressType::PUBLIC:
                return Napi::String::New(info.Env(), "public");
            case SimpleBLE::BluetoothAddressType::RANDOM:
                return Napi::String::New(info.Env(), "random");
            default:
                return Napi::String::New(info.Env(), "unspecified");
        }
    });
}

// NOTE: The core reports a missing RSSI or TX power as -32768; JavaScript gets
// undefined instead.
static Napi::Value optional_power(Napi::Env env, int16_t value) {
    if (value == std::numeric_limits<int16_t>::min()) return env.Undefined();
    return Napi::Number::New(env, value);
}

Napi::Value PeripheralHandle::rssi(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return optional_power(info.Env(), peripheral_.rssi()); });
}

Napi::Value PeripheralHandle::tx_power(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return optional_power(info.Env(), peripheral_.tx_power()); });
}

Napi::Value PeripheralHandle::mtu(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::Number::New(info.Env(), peripheral_.mtu()); });
}

Napi::Value PeripheralHandle::is_connected(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::Boolean::New(info.Env(), peripheral_.is_connected()); });
}

Napi::Value PeripheralHandle::is_connectable(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::Boolean::New(info.Env(), peripheral_.is_connectable()); });
}

// NOTE: Unlike SimplePyBLE's aio module, this returns a promise: a Dongl answers
// it over its serial port, and Windows queries the device.
Napi::Value PeripheralHandle::is_paired(const Napi::CallbackInfo& info) {
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral]() mutable { return finish_bool(peripheral.is_paired()); });
}

// Returns [companyId, Buffer] pairs; the TypeScript layer turns them into a Map.
Napi::Value PeripheralHandle::manufacturer_data(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] {
        auto data = peripheral_.manufacturer_data();
        auto array = Napi::Array::New(info.Env(), data.size());
        uint32_t i = 0;
        for (const auto& [company_id, bytes] : data) {
            auto pair = Napi::Array::New(info.Env(), 2);
            pair.Set(uint32_t(0), Napi::Number::New(info.Env(), company_id));
            pair.Set(uint32_t(1), to_buffer(info.Env(), bytes));
            array.Set(i++, pair);
        }
        return array;
    });
}

}  // namespace SimpleJSBLE
