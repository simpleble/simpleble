#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <simpleble/export.h>

#include <simpleble/Exceptions.h>
#include <simpleble/Types.h>

namespace SimpleBLE {

class LocalPeripheralBase;
class LocalServiceBase;
class LocalCharacteristicBase;

/**
 * Local BLE peripheral hosted by this process.
 *
 * `SimpleBLE::Peripheral` is a remote device discovered by this host.
 * `SimpleBLE::LocalPeripheral` is this host exposing local GATT services.
 */

enum class LocalCharacteristicCapability {
    READ,
    WRITE_REQUEST,
    WRITE_COMMAND,
    NOTIFY,
    INDICATE,
};

struct SIMPLEBLE_EXPORT LocalAdvertisement {
    std::optional<std::string> local_name;

    /**
     * If left empty, backends should advertise the primary service UUIDs when
     * the platform supports doing so.
     */
    std::vector<BluetoothUUID> service_uuids;
};

class SIMPLEBLE_EXPORT LocalCharacteristic {
  public:
    LocalCharacteristic() = default;
    virtual ~LocalCharacteristic() = default;

    bool initialized() const;

    BluetoothUUID uuid();
    std::vector<LocalCharacteristicCapability> capabilities();

    /**
     * Read or update the characteristic's current value.
     *
     * Updating the value of a characteristic with NOTIFY or INDICATE capability
     * propagates the change to subscribed clients.
     */
    ByteArray value();
    void set_value(ByteArray value);

    /**
     * Optional dynamic value callbacks.
     *
     * Without these callbacks, reads return `value()` and writes update
     * `value()` automatically.
     */
    void set_callback_on_read(std::function<ByteArray()> on_read);
    void set_callback_on_write(std::function<void(ByteArray value)> on_write);

    void set_callback_on_subscribed(std::function<void()> on_subscribed);
    void set_callback_on_unsubscribed(std::function<void()> on_unsubscribed);

  protected:
    LocalCharacteristicBase* operator->();
    const LocalCharacteristicBase* operator->() const;

    std::shared_ptr<LocalCharacteristicBase> internal_;
};

class SIMPLEBLE_EXPORT LocalService {
  public:
    LocalService() = default;
    virtual ~LocalService() = default;

    bool initialized() const;

    BluetoothUUID uuid();
    bool primary();

    LocalCharacteristic add_characteristic(BluetoothUUID uuid,
                                           std::vector<LocalCharacteristicCapability> capabilities);
    std::vector<LocalCharacteristic> characteristics();

  protected:
    LocalServiceBase* operator->();
    const LocalServiceBase* operator->() const;

    std::shared_ptr<LocalServiceBase> internal_;
};

class SIMPLEBLE_EXPORT LocalPeripheral {
  public:
    LocalPeripheral() = default;
    virtual ~LocalPeripheral() = default;

    bool initialized() const;
    void* underlying() const;

    LocalAdvertisement advertisement();
    void set_advertisement(LocalAdvertisement advertisement);

    LocalService add_service(BluetoothUUID uuid, bool primary = true);
    std::vector<LocalService> services();
    void remove_all_services();

    /**
     * Publish services and start advertising.
     *
     * Services and characteristics should be configured before calling start().
     */
    void start();
    void stop();

    bool is_started();
    bool is_advertising();

    void set_callback_on_client_connected(std::function<void()> on_client_connected);
    void set_callback_on_client_disconnected(std::function<void()> on_client_disconnected);

  protected:
    LocalPeripheralBase* operator->();
    const LocalPeripheralBase* operator->() const;

    std::shared_ptr<LocalPeripheralBase> internal_;
};

}  // namespace SimpleBLE
