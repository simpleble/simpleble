#pragma once

#include <napi.h>
#include <simpleble/Backend.h>
#include <simpleble/SimpleBLE.h>

#include <memory>
#include <string>

#include "Addon.h"

namespace SimpleJSBLE {

// Native callbacks hold this instead of a handle.
struct EventTarget {
    std::weak_ptr<Dispatcher> dispatcher;
    std::string key;
};

// The handles below are internal: the TypeScript classes own them and expose
// the public API. Each wraps a copy of the SimpleBLE object and releases it on
// the lifecycle strand when collected. Backend calls enumerate adapters, so they
// run on the lifecycle strand too; adapters and peripherals create their own
// strand on their first asynchronous call.

class BackendHandle : public Napi::ObjectWrap<BackendHandle> {
  public:
    static Napi::Function define(Napi::Env env);
    static Napi::Object wrap(Napi::Env env, const SimpleBLE::Backend& backend);

    explicit BackendHandle(const Napi::CallbackInfo& info);
    ~BackendHandle() override;

  private:
    Napi::Value key(const Napi::CallbackInfo& info);
    Napi::Value identifier(const Napi::CallbackInfo& info);
    Napi::Value bluetooth_enabled(const Napi::CallbackInfo& info);
    Napi::Value adapters(const Napi::CallbackInfo& info);

    SimpleBLE::Backend backend_;
};

class AdapterHandle : public Napi::ObjectWrap<AdapterHandle> {
  public:
    static Napi::Function define(Napi::Env env);
    static Napi::Object wrap(Napi::Env env, const SimpleBLE::Adapter& adapter);

    explicit AdapterHandle(const Napi::CallbackInfo& info);
    ~AdapterHandle() override;

  private:
    Napi::Value key(const Napi::CallbackInfo& info);
    Napi::Value identifier(const Napi::CallbackInfo& info);
    Napi::Value address(const Napi::CallbackInfo& info);
    Napi::Value is_powered(const Napi::CallbackInfo& info);
    Napi::Value power_on(const Napi::CallbackInfo& info);
    Napi::Value power_off(const Napi::CallbackInfo& info);
    Napi::Value scan_start(const Napi::CallbackInfo& info);
    Napi::Value scan_stop(const Napi::CallbackInfo& info);
    Napi::Value scan_for(const Napi::CallbackInfo& info);
    Napi::Value scan_is_active(const Napi::CallbackInfo& info);
    Napi::Value scan_get_results(const Napi::CallbackInfo& info);
    Napi::Value get_paired_peripherals(const Napi::CallbackInfo& info);
    Napi::Value get_connected_peripherals(const Napi::CallbackInfo& info);
    Napi::Value subscribe(const Napi::CallbackInfo& info);

    std::shared_ptr<Executor::Strand> strand();

    SimpleBLE::Adapter adapter_;
    std::shared_ptr<Executor::Strand> strand_;
};

class PeripheralHandle : public Napi::ObjectWrap<PeripheralHandle> {
  public:
    static Napi::Function define(Napi::Env env);
    static Napi::Object wrap(Napi::Env env, const SimpleBLE::Peripheral& peripheral);

    explicit PeripheralHandle(const Napi::CallbackInfo& info);
    ~PeripheralHandle() override;

  private:
    Napi::Value key(const Napi::CallbackInfo& info);
    Napi::Value identifier(const Napi::CallbackInfo& info);
    Napi::Value address(const Napi::CallbackInfo& info);
    Napi::Value address_type(const Napi::CallbackInfo& info);
    Napi::Value rssi(const Napi::CallbackInfo& info);
    Napi::Value tx_power(const Napi::CallbackInfo& info);
    Napi::Value mtu(const Napi::CallbackInfo& info);
    Napi::Value is_connected(const Napi::CallbackInfo& info);
    Napi::Value is_connectable(const Napi::CallbackInfo& info);
    Napi::Value is_paired(const Napi::CallbackInfo& info);
    Napi::Value manufacturer_data(const Napi::CallbackInfo& info);
    Napi::Value connect(const Napi::CallbackInfo& info);
    Napi::Value disconnect(const Napi::CallbackInfo& info);
    Napi::Value unpair(const Napi::CallbackInfo& info);
    Napi::Value services(const Napi::CallbackInfo& info);
    Napi::Value read(const Napi::CallbackInfo& info);
    Napi::Value write_request(const Napi::CallbackInfo& info);
    Napi::Value write_command(const Napi::CallbackInfo& info);
    Napi::Value read_descriptor(const Napi::CallbackInfo& info);
    Napi::Value write_descriptor(const Napi::CallbackInfo& info);
    Napi::Value notify(const Napi::CallbackInfo& info);
    Napi::Value indicate(const Napi::CallbackInfo& info);
    Napi::Value unsubscribe(const Napi::CallbackInfo& info);
    Napi::Value subscribe(const Napi::CallbackInfo& info);

    std::shared_ptr<Executor::Strand> strand();

    SimpleBLE::Peripheral peripheral_;
    std::shared_ptr<Executor::Strand> strand_;
};

Napi::Array wrap_adapters(Napi::Env env, const std::vector<SimpleBLE::Adapter>& adapters);
Napi::Array wrap_peripherals(Napi::Env env, const std::vector<SimpleBLE::Peripheral>& peripherals);

// Turns the External passed with a peripheral event into a handle.
Napi::Value peripheral_handle(const Napi::CallbackInfo& info);

}  // namespace SimpleJSBLE
