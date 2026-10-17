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
// the lifecycle strand when collected or closed. Backend calls enumerate
// adapters, so they run on the lifecycle strand too; adapters and peripherals
// create their own strand on their first asynchronous call.

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
    Napi::Value retrieve_cached_peripherals(const Napi::CallbackInfo& info);
    Napi::Value subscribe(const Napi::CallbackInfo& info);
    Napi::Value create_local_peripheral(const Napi::CallbackInfo& info);
    Napi::Value connections(const Napi::CallbackInfo& info);
    Napi::Value hosts(const Napi::CallbackInfo& info);
    Napi::Value close(const Napi::CallbackInfo& info);
    Napi::Value release(const Napi::CallbackInfo& info);

    std::shared_ptr<Executor::Strand> strand();

    SimpleBLE::Adapter adapter_;
    std::shared_ptr<Executor::Strand> strand_;
};

class PeripheralHandle : public Napi::ObjectWrap<PeripheralHandle> {
  public:
    static Napi::Function define(Napi::Env env);
    static Napi::Object wrap(Napi::Env env, const SimpleBLE::Peripheral& peripheral, const void* owner);

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
    Napi::Value set_passkey_request_handler(const Napi::CallbackInfo& info);
    Napi::Value set_passkey_display_handler(const Napi::CallbackInfo& info);
    Napi::Value set_numeric_comparison_handler(const Napi::CallbackInfo& info);
    Napi::Value owner(const Napi::CallbackInfo& info);
    Napi::Value release(const Napi::CallbackInfo& info);

    std::shared_ptr<Executor::Strand> strand();

    SimpleBLE::Peripheral peripheral_;
    // The adapter the peripheral came from, for Adapter.close().
    const void* owner_ = nullptr;
    std::shared_ptr<Executor::Strand> strand_;
};

// A local object, the adapter it was created from, and the local peripheral
// that hosts it, as passed to the local handles. Calls on a local object run
// on its host's strand.
template <typename T>
struct LocalSource {
    T object;
    const void* owner;
    const void* host;
};

class LocalPeripheralHandle : public Napi::ObjectWrap<LocalPeripheralHandle> {
  public:
    static Napi::Function define(Napi::Env env);
    static Napi::Object wrap(Napi::Env env, const SimpleBLE::Local::Peripheral& peripheral, const void* owner);

    explicit LocalPeripheralHandle(const Napi::CallbackInfo& info);
    ~LocalPeripheralHandle() override;

  private:
    Napi::Value key(const Napi::CallbackInfo& info);
    Napi::Value add_advertised_service(const Napi::CallbackInfo& info);
    Napi::Value set_advertised_manufacturer_data(const Napi::CallbackInfo& info);
    Napi::Value set_connectable(const Napi::CallbackInfo& info);
    Napi::Value set_advertisement_local_name(const Napi::CallbackInfo& info);
    Napi::Value add_service(const Napi::CallbackInfo& info);
    Napi::Value services(const Napi::CallbackInfo& info);
    Napi::Value remove_all_services(const Napi::CallbackInfo& info);
    Napi::Value start(const Napi::CallbackInfo& info);
    Napi::Value stop(const Napi::CallbackInfo& info);
    Napi::Value is_started(const Napi::CallbackInfo& info);
    Napi::Value is_advertising(const Napi::CallbackInfo& info);
    Napi::Value subscribe(const Napi::CallbackInfo& info);
    Napi::Value owner(const Napi::CallbackInfo& info);
    Napi::Value release(const Napi::CallbackInfo& info);

    std::shared_ptr<Executor::Strand> strand();

    SimpleBLE::Local::Peripheral peripheral_;
    const void* owner_ = nullptr;
    std::shared_ptr<Executor::Strand> strand_;
};

class LocalServiceHandle : public Napi::ObjectWrap<LocalServiceHandle> {
  public:
    static Napi::Function define(Napi::Env env);
    static Napi::Object wrap(Napi::Env env, const SimpleBLE::Local::Service& service, const void* owner,
                             const void* host);

    explicit LocalServiceHandle(const Napi::CallbackInfo& info);
    ~LocalServiceHandle() override;

  private:
    Napi::Value key(const Napi::CallbackInfo& info);
    Napi::Value uuid(const Napi::CallbackInfo& info);
    Napi::Value add_characteristic(const Napi::CallbackInfo& info);
    Napi::Value characteristics(const Napi::CallbackInfo& info);
    Napi::Value owner(const Napi::CallbackInfo& info);
    Napi::Value release(const Napi::CallbackInfo& info);

    SimpleBLE::Local::Service service_;
    const void* owner_ = nullptr;
    const void* host_ = nullptr;
};

class LocalCharacteristicHandle : public Napi::ObjectWrap<LocalCharacteristicHandle> {
  public:
    static Napi::Function define(Napi::Env env);
    static Napi::Object wrap(Napi::Env env, const SimpleBLE::Local::Characteristic& characteristic,
                             const void* owner, const void* host);

    explicit LocalCharacteristicHandle(const Napi::CallbackInfo& info);
    ~LocalCharacteristicHandle() override;

  private:
    Napi::Value key(const Napi::CallbackInfo& info);
    Napi::Value uuid(const Napi::CallbackInfo& info);
    Napi::Value capabilities(const Napi::CallbackInfo& info);
    Napi::Value value(const Napi::CallbackInfo& info);
    Napi::Value set_value(const Napi::CallbackInfo& info);
    Napi::Value set_read_handler(const Napi::CallbackInfo& info);
    Napi::Value subscribe(const Napi::CallbackInfo& info);
    Napi::Value owner(const Napi::CallbackInfo& info);
    Napi::Value release(const Napi::CallbackInfo& info);

    std::shared_ptr<Executor::Strand> strand();

    SimpleBLE::Local::Characteristic characteristic_;
    const void* owner_ = nullptr;
    const void* host_ = nullptr;
    std::shared_ptr<Executor::Strand> strand_;
};

// The id of the handler a call registers, or none to clear it.
std::optional<double> handler_id(const Napi::CallbackInfo& info, size_t index);

Napi::Array wrap_adapters(Napi::Env env, const std::vector<SimpleBLE::Adapter>& adapters);
Napi::Array wrap_peripherals(Napi::Env env, const std::vector<SimpleBLE::Peripheral>& peripherals, const void* owner);

// Turns the External passed with a peripheral event into a handle.
Napi::Value peripheral_handle(const Napi::CallbackInfo& info);

}  // namespace SimpleJSBLE
