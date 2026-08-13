#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <simpleble/export.h>

#include <simpleble/Exceptions.h>
#include <simpleble/Types.h>
#include <simpleble/local/Advertisement.h>
#include <simpleble/local/Service.h>

namespace SimpleBLE::Local {

class PeripheralBase;

/**
 * Local BLE peripheral hosted by this process.
 *
 * `SimpleBLE::Peripheral` is a remote device discovered by this host.
 * `SimpleBLE::Local::Peripheral` is this host exposing local GATT services.
 */
class SIMPLEBLE_EXPORT Peripheral {
  public:
    Peripheral() = default;
    virtual ~Peripheral() = default;

    bool initialized() const;
    void* underlying() const;

    Advertisement advertisement();
    void set_advertisement(Advertisement advertisement);

    /** Add a primary GATT service. */
    Service add_service(BluetoothUUID uuid);
    std::vector<Service> services();
    void remove_all_services();

    /**
     * Publish services and start advertising.
     *
     * Services, characteristics, and advertising data must be configured
     * before calling `start()` and cannot be changed while started.
     */
    void start();
    void stop();

    bool is_started();
    bool is_advertising();

    /**
     * Observe clients connecting to and disconnecting from this peripheral.
     *
     * On platforms that do not expose a Bluetooth address, the value is a
     * platform-specific identifier for the remote client.
     */
    void set_callback_on_client_connected(std::function<void(BluetoothAddress client_address)> on_client_connected);
    void set_callback_on_client_disconnected(
        std::function<void(BluetoothAddress client_address)> on_client_disconnected);

  protected:
    PeripheralBase* operator->();
    const PeripheralBase* operator->() const;

    std::shared_ptr<PeripheralBase> internal_;
};

}  // namespace SimpleBLE::Local
