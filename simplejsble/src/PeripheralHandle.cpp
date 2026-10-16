#include "Handles.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <string>

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
                           InstanceMethod<&PeripheralHandle::connect>("connect"),
                           InstanceMethod<&PeripheralHandle::disconnect>("disconnect"),
                           InstanceMethod<&PeripheralHandle::unpair>("unpair"),
                           InstanceMethod<&PeripheralHandle::services>("services"),
                           InstanceMethod<&PeripheralHandle::read>("read"),
                           InstanceMethod<&PeripheralHandle::write_request>("writeRequest"),
                           InstanceMethod<&PeripheralHandle::write_command>("writeCommand"),
                           InstanceMethod<&PeripheralHandle::read_descriptor>("readDescriptor"),
                           InstanceMethod<&PeripheralHandle::write_descriptor>("writeDescriptor"),
                           InstanceMethod<&PeripheralHandle::notify>("notify"),
                           InstanceMethod<&PeripheralHandle::indicate>("indicate"),
                           InstanceMethod<&PeripheralHandle::unsubscribe>("unsubscribe"),
                           InstanceMethod<&PeripheralHandle::subscribe>("subscribe"),
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

Napi::Value peripheral_handle(const Napi::CallbackInfo& info) {
    if (!info[0].IsExternal()) throw Napi::TypeError::New(info.Env(), "Expected a peripheral");
    return PeripheralHandle::wrap(info.Env(), *info[0].As<Napi::External<SimpleBLE::Peripheral>>().Data());
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

namespace {

std::string uuid_argument(const Napi::CallbackInfo& info, size_t index) {
    if (!info[index].IsString()) throw Napi::TypeError::New(info.Env(), "UUIDs must be strings");
    return info[index].As<Napi::String>().Utf8Value();
}

// Delivers a notification or indication to the TypeScript side, which calls
// the function registered under `id`.
std::function<void(SimpleBLE::ByteArray)> deliverer(const std::weak_ptr<Dispatcher>& dispatcher, double id) {
    return [dispatcher, id](SimpleBLE::ByteArray value) {
        auto target = dispatcher.lock();
        if (!target) return;
        target->post([id, value](Napi::Env env) {
            addon(env).deliver.Call({Napi::Number::New(env, id), to_buffer(env, value)});
        });
    };
}

double subscription_id(const Napi::CallbackInfo& info, size_t index) {
    if (!info[index].IsNumber()) throw Napi::TypeError::New(info.Env(), "Expected a subscription id");
    return info[index].As<Napi::Number>().DoubleValue();
}

}  // namespace

Napi::Value PeripheralHandle::connect(const Napi::CallbackInfo& info) {
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral]() mutable -> Finisher {
        peripheral.connect();
        return [peripheral](Napi::Env env) {
            hold_active(env, addon(env).connected, Internal<SimpleBLE::Peripheral>::of(peripheral),
                        Internal<SimpleBLE::Peripheral>::pointer(peripheral));
            return env.Undefined();
        };
    });
}

Napi::Value PeripheralHandle::disconnect(const Napi::CallbackInfo& info) {
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral]() mutable -> Finisher {
        peripheral.disconnect();
        return [peripheral](Napi::Env env) {
            release_active(env, addon(env).connected, Internal<SimpleBLE::Peripheral>::of(peripheral));
            return env.Undefined();
        };
    });
}

Napi::Value PeripheralHandle::unpair(const Napi::CallbackInfo& info) {
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral]() mutable {
        peripheral.unpair();
        return finish_undefined();
    });
}

Napi::Value PeripheralHandle::services(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    return guard(env, [&] {
        auto services = peripheral_.services();
        auto service_array = Napi::Array::New(env, services.size());
        for (uint32_t i = 0; i < services.size(); i++) {
            auto& service = services[i];
            auto characteristics = service.characteristics();
            auto characteristic_array = Napi::Array::New(env, characteristics.size());
            for (uint32_t j = 0; j < characteristics.size(); j++) {
                auto& characteristic = characteristics[j];

                auto capabilities = Napi::Array::New(env);
                uint32_t count = 0;
                if (characteristic.can_read()) capabilities.Set(count++, "read");
                if (characteristic.can_write_request()) capabilities.Set(count++, "writeRequest");
                if (characteristic.can_write_command()) capabilities.Set(count++, "writeCommand");
                if (characteristic.can_notify()) capabilities.Set(count++, "notify");
                if (characteristic.can_indicate()) capabilities.Set(count++, "indicate");

                auto descriptors = characteristic.descriptors();
                auto descriptor_array = Napi::Array::New(env, descriptors.size());
                for (uint32_t k = 0; k < descriptors.size(); k++) {
                    auto descriptor = Napi::Object::New(env);
                    descriptor.Set("uuid", descriptors[k].uuid());
                    descriptor_array.Set(k, descriptor);
                }

                auto object = Napi::Object::New(env);
                object.Set("uuid", characteristic.uuid());
                object.Set("capabilities", capabilities);
                object.Set("descriptors", descriptor_array);
                characteristic_array.Set(j, object);
            }

            auto object = Napi::Object::New(env);
            object.Set("uuid", service.uuid());
            object.Set("data", to_buffer(env, service.data()));
            object.Set("characteristics", characteristic_array);
            service_array.Set(i, object);
        }
        return service_array;
    });
}

Napi::Value PeripheralHandle::read(const Napi::CallbackInfo& info) {
    auto service = uuid_argument(info, 0);
    auto characteristic = uuid_argument(info, 1);
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral, service, characteristic]() mutable -> Finisher {
        auto value = peripheral.read(service, characteristic);
        return [value](Napi::Env env) { return to_buffer(env, value); };
    });
}

Napi::Value PeripheralHandle::write_request(const Napi::CallbackInfo& info) {
    auto service = uuid_argument(info, 0);
    auto characteristic = uuid_argument(info, 1);
    auto value = to_bytes(info[2]);
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral, service, characteristic, value]() mutable {
        peripheral.write_request(service, characteristic, value);
        return finish_undefined();
    });
}

Napi::Value PeripheralHandle::write_command(const Napi::CallbackInfo& info) {
    auto service = uuid_argument(info, 0);
    auto characteristic = uuid_argument(info, 1);
    auto value = to_bytes(info[2]);
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral, service, characteristic, value]() mutable {
        peripheral.write_command(service, characteristic, value);
        return finish_undefined();
    });
}

Napi::Value PeripheralHandle::read_descriptor(const Napi::CallbackInfo& info) {
    auto service = uuid_argument(info, 0);
    auto characteristic = uuid_argument(info, 1);
    auto descriptor = uuid_argument(info, 2);
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral, service, characteristic, descriptor]() mutable -> Finisher {
        auto value = peripheral.read(service, characteristic, descriptor);
        return [value](Napi::Env env) { return to_buffer(env, value); };
    });
}

Napi::Value PeripheralHandle::write_descriptor(const Napi::CallbackInfo& info) {
    auto service = uuid_argument(info, 0);
    auto characteristic = uuid_argument(info, 1);
    auto descriptor = uuid_argument(info, 2);
    auto value = to_bytes(info[3]);
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral, service, characteristic, descriptor, value]() mutable {
        peripheral.write(service, characteristic, descriptor, value);
        return finish_undefined();
    });
}

Napi::Value PeripheralHandle::notify(const Napi::CallbackInfo& info) {
    auto service = uuid_argument(info, 0);
    auto characteristic = uuid_argument(info, 1);
    auto callback = deliverer(addon(info.Env()).dispatcher, subscription_id(info, 2));
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral, service, characteristic, callback]() mutable {
        peripheral.notify(service, characteristic, callback);
        return finish_undefined();
    });
}

Napi::Value PeripheralHandle::indicate(const Napi::CallbackInfo& info) {
    auto service = uuid_argument(info, 0);
    auto characteristic = uuid_argument(info, 1);
    auto callback = deliverer(addon(info.Env()).dispatcher, subscription_id(info, 2));
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral, service, characteristic, callback]() mutable {
        peripheral.indicate(service, characteristic, callback);
        return finish_undefined();
    });
}

Napi::Value PeripheralHandle::unsubscribe(const Napi::CallbackInfo& info) {
    auto service = uuid_argument(info, 0);
    auto characteristic = uuid_argument(info, 1);
    auto peripheral = peripheral_;
    return run(info.Env(), strand(), [peripheral, service, characteristic]() mutable {
        peripheral.unsubscribe(service, characteristic);
        return finish_undefined();
    });
}

// Called once, when the TypeScript Peripheral is created, like the adapter's.
Napi::Value PeripheralHandle::subscribe(const Napi::CallbackInfo& info) {
    auto events = std::make_shared<EventTarget>();
    events->dispatcher = addon(info.Env()).dispatcher;
    events->key = key_of(Internal<SimpleBLE::Peripheral>::of(peripheral_));

    // NOTE: The callbacks are stored in the native peripheral, so they hold it weakly.
    const void* pointer = Internal<SimpleBLE::Peripheral>::of(peripheral_);
    std::weak_ptr<void> native = Internal<SimpleBLE::Peripheral>::pointer(peripheral_);

    auto on_connected = [events, pointer, native]() {
        auto dispatcher = events->dispatcher.lock();
        if (!dispatcher) return;
        dispatcher->post([events, pointer, native](Napi::Env env) {
            hold_active(env, addon(env).connected, pointer, native.lock());
            emit(env, events->key, "connected");
        });
    };

    auto on_disconnected = [events, pointer]() {
        auto dispatcher = events->dispatcher.lock();
        if (!dispatcher) return;
        dispatcher->post([events, pointer](Napi::Env env) {
            release_active(env, addon(env).connected, pointer);
            emit(env, events->key, "disconnected");
        });
    };

    return guard(info.Env(), [&] {
        peripheral_.set_callback_on_connected(on_connected);
        peripheral_.set_callback_on_disconnected(on_disconnected);
        return info.Env().Undefined();
    });
}

}  // namespace SimpleJSBLE
