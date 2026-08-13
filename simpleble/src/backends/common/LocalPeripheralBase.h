#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <simpleble/Types.h>
#include <simpleble/local/Advertisement.h>

namespace SimpleBLE::Local {

class ServiceBase;

class PeripheralBase {
  public:
    virtual ~PeripheralBase() = default;

    virtual void* underlying() const = 0;

    virtual Advertisement advertisement() = 0;
    virtual void set_advertisement(Advertisement advertisement) = 0;

    virtual std::shared_ptr<ServiceBase> add_service(BluetoothUUID uuid) = 0;
    virtual std::vector<std::shared_ptr<ServiceBase>> services() = 0;
    virtual void remove_all_services() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual bool is_started() = 0;
    virtual bool is_advertising() = 0;

    virtual void set_callback_on_client_connected(
        std::function<void(BluetoothAddress client_address)> on_client_connected) = 0;
    virtual void set_callback_on_client_disconnected(
        std::function<void(BluetoothAddress client_address)> on_client_disconnected) = 0;

  protected:
    PeripheralBase() = default;
};

}  // namespace SimpleBLE::Local
