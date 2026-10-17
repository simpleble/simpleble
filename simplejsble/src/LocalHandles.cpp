#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <simpleble/Advanced.h>

#include "Handles.h"

namespace SimpleJSBLE {

namespace {

using Capability = SimpleBLE::Local::CharacteristicCapability;

// In the order capabilities() lists them.
const std::pair<const char*, Capability> CAPABILITIES[] = {
    {"read", Capability::READ},
    {"writeRequest", Capability::WRITE_REQUEST},
    {"writeCommand", Capability::WRITE_COMMAND},
    {"notify", Capability::NOTIFY},
    {"indicate", Capability::INDICATE},
};

std::set<Capability> to_capabilities(const Napi::Value& value) {
    if (!value.IsArray()) throw Napi::TypeError::New(value.Env(), "capabilities must be an array");

    auto array = value.As<Napi::Array>();
    std::set<Capability> capabilities;
    for (uint32_t i = 0; i < array.Length(); i++) {
        Napi::Value item = array.Get(i);
        const std::string name = item.IsString() ? item.As<Napi::String>().Utf8Value() : "";
        auto match = std::find_if(std::begin(CAPABILITIES), std::end(CAPABILITIES),
                                  [&](const auto& capability) { return name == capability.first; });
        if (match == std::end(CAPABILITIES)) throw Napi::TypeError::New(value.Env(), "Unknown capability: " + name);
        capabilities.insert(match->second);
    }
    return capabilities;
}

std::string string_argument(const Napi::CallbackInfo& info, size_t index, const char* message) {
    if (!info[index].IsString()) throw Napi::TypeError::New(info.Env(), message);
    return info[index].As<Napi::String>().Utf8Value();
}

template <typename T>
const T& source(const Napi::CallbackInfo& info, const char* message) {
    if (!info[0].IsExternal()) throw Napi::TypeError::New(info.Env(), message);
    return *info[0].As<Napi::External<T>>().Data();
}

template <typename T>
Napi::Object construct(Napi::FunctionReference& constructor, Napi::Env env, const T& object, const void* owner,
                       const void* host) {
    LocalSource<T> source{object, owner, host};
    return constructor.New({Napi::External<LocalSource<T>>::New(env, &source)});
}

std::shared_ptr<EventTarget> event_target(Napi::Env env, const void* pointer) {
    auto events = std::make_shared<EventTarget>();
    events->dispatcher = addon(env).dispatcher;
    events->key = key_of(pointer);
    return events;
}

}  // namespace

std::optional<double> handler_id(const Napi::CallbackInfo& info, size_t index) {
    if (info[index].IsUndefined() || info[index].IsNull()) return std::nullopt;
    if (!info[index].IsNumber()) throw Napi::TypeError::New(info.Env(), "Expected a handler id");
    return info[index].As<Napi::Number>().DoubleValue();
}

// Local peripheral.

Napi::Function LocalPeripheralHandle::define(Napi::Env env) {
    return DefineClass(env, "LocalPeripheralHandle",
                       {
                           InstanceMethod<&LocalPeripheralHandle::key>("key"),
                           InstanceMethod<&LocalPeripheralHandle::add_advertised_service>("addAdvertisedService"),
                           InstanceMethod<&LocalPeripheralHandle::set_advertised_manufacturer_data>(
                               "setAdvertisedManufacturerData"),
                           InstanceMethod<&LocalPeripheralHandle::set_connectable>("setConnectable"),
                           InstanceMethod<&LocalPeripheralHandle::set_advertisement_local_name>(
                               "setAdvertisementLocalName"),
                           InstanceMethod<&LocalPeripheralHandle::add_service>("addService"),
                           InstanceMethod<&LocalPeripheralHandle::services>("services"),
                           InstanceMethod<&LocalPeripheralHandle::remove_all_services>("removeAllServices"),
                           InstanceMethod<&LocalPeripheralHandle::start>("start"),
                           InstanceMethod<&LocalPeripheralHandle::stop>("stop"),
                           InstanceMethod<&LocalPeripheralHandle::is_started>("isStarted"),
                           InstanceMethod<&LocalPeripheralHandle::is_advertising>("isAdvertising"),
                           InstanceMethod<&LocalPeripheralHandle::subscribe>("subscribe"),
                           InstanceMethod<&LocalPeripheralHandle::owner>("owner"),
                           InstanceMethod<&LocalPeripheralHandle::release>("release"),
                       });
}

Napi::Object LocalPeripheralHandle::wrap(Napi::Env env, const SimpleBLE::Local::Peripheral& peripheral,
                                         const void* owner) {
    return construct(addon(env).local_peripheral_constructor, env, peripheral, owner,
                                            Internal<SimpleBLE::Local::Peripheral>::of(peripheral));
}

LocalPeripheralHandle::LocalPeripheralHandle(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<LocalPeripheralHandle>(info) {
    const auto& from = source<LocalSource<SimpleBLE::Local::Peripheral>>(info, "Use Adapter.createLocalPeripheral()");
    peripheral_ = from.object;
    owner_ = from.owner;
}

LocalPeripheralHandle::~LocalPeripheralHandle() { release_later(peripheral_); }

std::shared_ptr<Executor::Strand> LocalPeripheralHandle::strand() {
    if (!strand_) strand_ = strand_for(Internal<SimpleBLE::Local::Peripheral>::of(peripheral_));
    return strand_;
}

Napi::Value LocalPeripheralHandle::key(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(Internal<SimpleBLE::Local::Peripheral>::of(peripheral_)));
}

Napi::Value LocalPeripheralHandle::add_advertised_service(const Napi::CallbackInfo& info) {
    std::vector<SimpleBLE::BluetoothUUID> uuids;
    if (info[0].IsArray()) {
        auto array = info[0].As<Napi::Array>();
        for (uint32_t i = 0; i < array.Length(); i++) {
            Napi::Value item = array.Get(i);
            if (!item.IsString()) throw Napi::TypeError::New(info.Env(), "UUIDs must be strings");
            uuids.push_back(item.As<Napi::String>().Utf8Value());
        }
    } else {
        uuids.push_back(string_argument(info, 0, "UUIDs must be strings"));
    }
    return guard(info.Env(), [&] {
        peripheral_.add_advertised_service(uuids);
        return info.Env().Undefined();
    });
}

Napi::Value LocalPeripheralHandle::set_advertised_manufacturer_data(const Napi::CallbackInfo& info) {
    const double company_id = info[0].IsNumber() ? info[0].As<Napi::Number>().DoubleValue() : -1;
    if (!(company_id >= 0 && company_id <= UINT16_MAX) || std::floor(company_id) != company_id) {
        throw Napi::RangeError::New(info.Env(), "companyId must be an integer from 0 to 65535");
    }
    auto data = to_bytes(info[1]);
    return guard(info.Env(), [&] {
        peripheral_.set_advertised_manufacturer_data(static_cast<uint16_t>(company_id), data);
        return info.Env().Undefined();
    });
}

Napi::Value LocalPeripheralHandle::set_connectable(const Napi::CallbackInfo& info) {
    if (!info[0].IsBoolean()) throw Napi::TypeError::New(info.Env(), "connectable must be a boolean");
    return guard(info.Env(), [&] {
        peripheral_.set_connectable(info[0].As<Napi::Boolean>().Value());
        return info.Env().Undefined();
    });
}

// NOTE: Only BlueZ and CoreBluetooth let the advertisement carry its own local name.
Napi::Value LocalPeripheralHandle::set_advertisement_local_name(const Napi::CallbackInfo& info) {
    std::optional<std::string> name;
    if (!info[0].IsNull()) name = string_argument(info, 0, "name must be a string or null");
    return guard(info.Env(), [&]() -> Napi::Value {
#if SIMPLEBLE_BACKEND_MACOS
        SimpleBLE::Advanced::MacOS::set_advertisement_local_name(peripheral_, name);
        return info.Env().Undefined();
#elif SIMPLEBLE_BACKEND_LINUX
        SimpleBLE::Advanced::Linux::set_advertisement_local_name(peripheral_, name);
        return info.Env().Undefined();
#else
        throw SimpleBLE::Exception::OperationNotSupported();
#endif
    });
}

Napi::Value LocalPeripheralHandle::add_service(const Napi::CallbackInfo& info) {
    auto uuid = string_argument(info, 0, "UUIDs must be strings");
    const void* host = Internal<SimpleBLE::Local::Peripheral>::of(peripheral_);
    return guard(info.Env(), [&] { return LocalServiceHandle::wrap(info.Env(), peripheral_.add_service(uuid), owner_, host); });
}

Napi::Value LocalPeripheralHandle::services(const Napi::CallbackInfo& info) {
    const void* host = Internal<SimpleBLE::Local::Peripheral>::of(peripheral_);
    return guard(info.Env(), [&] {
        auto services = peripheral_.services();
        auto array = Napi::Array::New(info.Env(), services.size());
        for (uint32_t i = 0; i < services.size(); i++) {
            array.Set(i, LocalServiceHandle::wrap(info.Env(), services[i], owner_, host));
        }
        return array;
    });
}

Napi::Value LocalPeripheralHandle::remove_all_services(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] {
        peripheral_.remove_all_services();
        return info.Env().Undefined();
    });
}

// NOTE: Like a listening server, a started local peripheral keeps Node running
// until stop() or its adapter's close().
Napi::Value LocalPeripheralHandle::start(const Napi::CallbackInfo& info) {
    auto peripheral = peripheral_;
    const void* owner = owner_;
    return run(info.Env(), strand(), [peripheral, owner]() mutable -> Finisher {
        peripheral.start();
        return [peripheral, owner](Napi::Env env) {
            hold_active(env, addon(env).hosting, peripheral, owner);
            return env.Undefined();
        };
    });
}

Napi::Value LocalPeripheralHandle::stop(const Napi::CallbackInfo& info) {
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral]() mutable -> Finisher {
        peripheral.stop();
        return [peripheral](Napi::Env env) {
            release_active(env, addon(env).hosting, Internal<SimpleBLE::Local::Peripheral>::of(peripheral));
            return env.Undefined();
        };
    });
}

Napi::Value LocalPeripheralHandle::is_started(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::Boolean::New(info.Env(), peripheral_.is_started()); });
}

// NOTE: A promise, as BlueZ answers it under the lock start() holds, and
// Windows on its WinRT thread.
Napi::Value LocalPeripheralHandle::is_advertising(const Napi::CallbackInfo& info) {
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral]() mutable { return finish_bool(peripheral.is_advertising()); });
}

// Called once, when the TypeScript LocalPeripheral is created.
Napi::Value LocalPeripheralHandle::subscribe(const Napi::CallbackInfo& info) {
    auto events = event_target(info.Env(), Internal<SimpleBLE::Local::Peripheral>::of(peripheral_));
    auto signal = [events](const char* event) {
        return [events, event](SimpleBLE::BluetoothAddress address) {
            auto dispatcher = events->dispatcher.lock();
            if (!dispatcher) return;
            dispatcher->post([events, event, address](Napi::Env env) {
                emit(env, events->key, event, Napi::String::New(env, address));
            });
        };
    };
    return guard(info.Env(), [&] {
        peripheral_.set_callback_on_client_connected(signal("clientConnected"));
        peripheral_.set_callback_on_client_disconnected(signal("clientDisconnected"));
        return info.Env().Undefined();
    });
}

Napi::Value LocalPeripheralHandle::owner(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(owner_));
}

Napi::Value LocalPeripheralHandle::release(const Napi::CallbackInfo& info) {
    release_later(peripheral_);
    return info.Env().Undefined();
}

// Local service.

Napi::Function LocalServiceHandle::define(Napi::Env env) {
    return DefineClass(env, "LocalServiceHandle",
                       {
                           InstanceMethod<&LocalServiceHandle::key>("key"),
                           InstanceMethod<&LocalServiceHandle::uuid>("uuid"),
                           InstanceMethod<&LocalServiceHandle::add_characteristic>("addCharacteristic"),
                           InstanceMethod<&LocalServiceHandle::characteristics>("characteristics"),
                           InstanceMethod<&LocalServiceHandle::owner>("owner"),
                           InstanceMethod<&LocalServiceHandle::release>("release"),
                       });
}

Napi::Object LocalServiceHandle::wrap(Napi::Env env, const SimpleBLE::Local::Service& service, const void* owner,
                                      const void* host) {
    return construct(addon(env).local_service_constructor, env, service, owner, host);
}

LocalServiceHandle::LocalServiceHandle(const Napi::CallbackInfo& info) : Napi::ObjectWrap<LocalServiceHandle>(info) {
    const auto& from = source<LocalSource<SimpleBLE::Local::Service>>(info, "Use LocalPeripheral.addService()");
    service_ = from.object;
    owner_ = from.owner;
    host_ = from.host;
}

LocalServiceHandle::~LocalServiceHandle() { release_later(service_); }

Napi::Value LocalServiceHandle::key(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(Internal<SimpleBLE::Local::Service>::of(service_)));
}

Napi::Value LocalServiceHandle::uuid(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::String::New(info.Env(), service_.uuid()); });
}

Napi::Value LocalServiceHandle::add_characteristic(const Napi::CallbackInfo& info) {
    auto uuid = string_argument(info, 0, "UUIDs must be strings");
    auto capabilities = to_capabilities(info[1]);
    return guard(info.Env(), [&] {
        return LocalCharacteristicHandle::wrap(info.Env(), service_.add_characteristic(uuid, capabilities), owner_,
                                               host_);
    });
}

Napi::Value LocalServiceHandle::characteristics(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] {
        auto characteristics = service_.characteristics();
        auto array = Napi::Array::New(info.Env(), characteristics.size());
        for (uint32_t i = 0; i < characteristics.size(); i++) {
            array.Set(i, LocalCharacteristicHandle::wrap(info.Env(), characteristics[i], owner_, host_));
        }
        return array;
    });
}

Napi::Value LocalServiceHandle::owner(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(owner_));
}

Napi::Value LocalServiceHandle::release(const Napi::CallbackInfo& info) {
    release_later(service_);
    return info.Env().Undefined();
}

// Local characteristic.

Napi::Function LocalCharacteristicHandle::define(Napi::Env env) {
    return DefineClass(env, "LocalCharacteristicHandle",
                       {
                           InstanceMethod<&LocalCharacteristicHandle::key>("key"),
                           InstanceMethod<&LocalCharacteristicHandle::uuid>("uuid"),
                           InstanceMethod<&LocalCharacteristicHandle::capabilities>("capabilities"),
                           InstanceMethod<&LocalCharacteristicHandle::value>("value"),
                           InstanceMethod<&LocalCharacteristicHandle::set_value>("setValue"),
                           InstanceMethod<&LocalCharacteristicHandle::set_read_handler>("setReadHandler"),
                           InstanceMethod<&LocalCharacteristicHandle::subscribe>("subscribe"),
                           InstanceMethod<&LocalCharacteristicHandle::owner>("owner"),
                           InstanceMethod<&LocalCharacteristicHandle::release>("release"),
                       });
}

Napi::Object LocalCharacteristicHandle::wrap(Napi::Env env, const SimpleBLE::Local::Characteristic& characteristic,
                                             const void* owner, const void* host) {
    return construct(addon(env).local_characteristic_constructor, env, characteristic,
                                                owner, host);
}

LocalCharacteristicHandle::LocalCharacteristicHandle(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<LocalCharacteristicHandle>(info) {
    const auto& from =
        source<LocalSource<SimpleBLE::Local::Characteristic>>(info, "Use LocalService.addCharacteristic()");
    characteristic_ = from.object;
    owner_ = from.owner;
    host_ = from.host;
}

LocalCharacteristicHandle::~LocalCharacteristicHandle() { release_later(characteristic_); }

std::shared_ptr<Executor::Strand> LocalCharacteristicHandle::strand() {
    if (!strand_) strand_ = strand_for(host_);
    return strand_;
}

Napi::Value LocalCharacteristicHandle::key(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(Internal<SimpleBLE::Local::Characteristic>::of(characteristic_)));
}

Napi::Value LocalCharacteristicHandle::uuid(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return Napi::String::New(info.Env(), characteristic_.uuid()); });
}

Napi::Value LocalCharacteristicHandle::capabilities(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] {
        auto capabilities = characteristic_.capabilities();
        auto array = Napi::Array::New(info.Env());
        uint32_t count = 0;
        for (const auto& [name, capability] : CAPABILITIES) {
            if (capabilities.count(capability)) array.Set(count++, name);
        }
        return array;
    });
}

Napi::Value LocalCharacteristicHandle::value(const Napi::CallbackInfo& info) {
    return guard(info.Env(), [&] { return to_buffer(info.Env(), characteristic_.value()); });
}

Napi::Value LocalCharacteristicHandle::set_value(const Napi::CallbackInfo& info) {
    auto value = to_bytes(info[0]);
    auto characteristic = characteristic_;
    return run(info.Env(), strand(), [characteristic, value]() mutable {
        characteristic.set_value(value);
        return finish_undefined();
    });
}

// NOTE: The core holds a callback's lock while it runs, and a read waits for
// the JavaScript thread, so the handler is replaced on the host's strand rather
// than the JavaScript thread.
Napi::Value LocalCharacteristicHandle::set_read_handler(const Napi::CallbackInfo& info) {
    auto id = handler_id(info, 0);
    std::weak_ptr dispatcher = addon(info.Env()).dispatcher;
    auto characteristic = characteristic_;
    return run(info.Env(), strand(), [characteristic, id, dispatcher]() mutable {
        std::function<SimpleBLE::ByteArray()> on_read;
        if (id) {
            on_read = [dispatcher, id = *id]() {
                return ask(dispatcher, id, std::nullopt, SimpleBLE::ByteArray(), to_bytes);
            };
        }
        characteristic.set_callback_on_read(on_read);
        return finish_undefined();
    });
}

// Called once, when the TypeScript LocalCharacteristic is created.
Napi::Value LocalCharacteristicHandle::subscribe(const Napi::CallbackInfo& info) {
    auto events = event_target(info.Env(), Internal<SimpleBLE::Local::Characteristic>::of(characteristic_));
    auto signal = [events](const char* event) {
        return [events, event]() {
            auto dispatcher = events->dispatcher.lock();
            if (!dispatcher) return;
            dispatcher->post([events, event](Napi::Env env) { emit(env, events->key, event); });
        };
    };
    auto on_write = [events](SimpleBLE::ByteArray value) {
        auto dispatcher = events->dispatcher.lock();
        if (!dispatcher) return;
        dispatcher->post([events, value](Napi::Env env) { emit(env, events->key, "write", to_buffer(env, value)); });
    };
    return guard(info.Env(), [&] {
        characteristic_.set_callback_on_write(on_write);
        characteristic_.set_callback_on_subscribed(signal("subscribed"));
        characteristic_.set_callback_on_unsubscribed(signal("unsubscribed"));
        return info.Env().Undefined();
    });
}

Napi::Value LocalCharacteristicHandle::owner(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), key_of(owner_));
}

Napi::Value LocalCharacteristicHandle::release(const Napi::CallbackInfo& info) {
    release_later(characteristic_);
    return info.Env().Undefined();
}

}  // namespace SimpleJSBLE
