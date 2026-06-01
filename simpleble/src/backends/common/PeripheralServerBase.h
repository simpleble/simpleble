#pragma once

#include <functional>
#include <vector>

#include <kvn_safe_callback.hpp>

#include <simpleble/PeripheralServer.h>

namespace SimpleBLE {

class PeripheralServerBase {
  public:
    virtual ~PeripheralServerBase() = default;

    virtual void* underlying() const = 0;

    virtual void add_service(ServiceDefinition service) = 0;
    virtual void remove_service(BluetoothUUID const& service) = 0;
    virtual std::vector<ServiceDefinition> services() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool is_active() = 0;

    virtual void start_advertising(AdvertisingData advertising_data) = 0;
    virtual void stop_advertising() = 0;
    virtual bool is_advertising() = 0;

    virtual void notify(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) = 0;
    virtual void notify(Central const& central, BluetoothUUID const& service, BluetoothUUID const& characteristic,
                        ByteArray const& data) = 0;
    virtual void indicate(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) = 0;
    virtual void indicate(Central const& central, BluetoothUUID const& service, BluetoothUUID const& characteristic,
                          ByteArray const& data) = 0;

    virtual std::vector<Central> centrals() = 0;

    virtual void set_callback_on_started(std::function<void()> on_started);
    virtual void set_callback_on_stopped(std::function<void()> on_stopped);
    virtual void set_callback_on_advertising_started(std::function<void()> on_started);
    virtual void set_callback_on_advertising_stopped(std::function<void()> on_stopped);
    virtual void set_callback_on_central_connected(std::function<void(Central)> on_connected);
    virtual void set_callback_on_central_disconnected(std::function<void(Central)> on_disconnected);

  protected:
    PeripheralServerBase() = default;

    kvn::safe_callback<void()> _callback_on_started;
    kvn::safe_callback<void()> _callback_on_stopped;
    kvn::safe_callback<void()> _callback_on_advertising_started;
    kvn::safe_callback<void()> _callback_on_advertising_stopped;
    kvn::safe_callback<void(Central)> _callback_on_central_connected;
    kvn::safe_callback<void(Central)> _callback_on_central_disconnected;
};

}  // namespace SimpleBLE
