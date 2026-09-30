#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <simpleble/Types.h>
#include <simpleble/export.h>
#include <simpleble/simulation/Connection.h>
#include <simpleble/simulation/Types.h>

namespace SimpleBLE::Simulation::Internal {
class DeviceCore;
}

namespace SimpleBLE::Simulation {

/**
 * A simulated peripheral.
 *
 * Subclass it to give the device behavior. The device declares its GATT
 * database with its own methods, addressed by UUID, and every event reaches
 * the device through the virtual `on_*` methods. Events for one device never
 * run concurrently, like callbacks in single-threaded firmware.
 *
 * Configuration takes effect for later advertisements and connections.
 */
class SIMPLEBLE_EXPORT Device {
  public:
    Device(std::string name, BluetoothAddress address);
    virtual ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    std::string name() const;
    BluetoothAddress address() const;

    // ---- Advertising ----------------------------------------------------------------------------------------------

    /** Advertising events occur every interval plus a random 0-10 ms delay. Default: 100 ms. */
    void set_advertising_interval(std::chrono::milliseconds interval);
    /** Default: true. */
    void set_connectable(bool connectable);
    /** Advertised TX power in dBm. Not advertised by default. */
    void set_tx_power(int16_t tx_power);
    void add_advertised_service(const BluetoothUUID& service);
    void set_manufacturer_data(uint16_t company_id, const ByteArray& data);
    void set_service_data(const BluetoothUUID& service, const ByteArray& data);

    // ---- Link layer -----------------------------------------------------------------------------------------------

    /**
     * Interval range the device asks for right after connecting, if the
     * central chose an interval outside it. Default: no preference.
     */
    void set_preferred_connection_interval(std::chrono::microseconds min, std::chrono::microseconds max);
    /** Largest ATT MTU the device accepts during the MTU exchange. Default: 247. */
    void set_max_mtu(uint16_t mtu);
    /** Largest link-layer payload in octets, 27 to 251. Default: 27. */
    void set_max_data_length(uint16_t octets);
    /** Longest the device spends in one connection event. Takes effect immediately. Default: 7.5 ms. */
    void set_max_event_length(std::chrono::microseconds length);
    /** Notifications and indications that can be queued per connection. Default: 4. */
    void set_tx_buffers(size_t count);
    /** Centrals connected at once. Below the limit the device keeps advertising. Default: 1. */
    void set_max_connections(size_t count);

    // ---- GATT database --------------------------------------------------------------------------------------------

    void add_service(const BluetoothUUID& service);
    void add_characteristic(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                            std::set<Property> properties);
    /**
     * Add a descriptor. The Client Characteristic Configuration descriptor is
     * managed by the simulation and added automatically to characteristics
     * that notify or indicate.
     */
    void add_descriptor(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                        const BluetoothUUID& descriptor);

    ByteArray value(const BluetoothUUID& service, const BluetoothUUID& characteristic) const;
    void set_value(const BluetoothUUID& service, const BluetoothUUID& characteristic, const ByteArray& value);
    ByteArray value(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                    const BluetoothUUID& descriptor) const;
    void set_value(const BluetoothUUID& service, const BluetoothUUID& characteristic, const BluetoothUUID& descriptor,
                   const ByteArray& value);

    // ---- Actions --------------------------------------------------------------------------------------------------

    std::vector<Connection> connections() const;

    /** Queue a notification. Values longer than MTU - 3 are truncated. */
    TxStatus notify(const Connection& connection, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                    const ByteArray& value);
    /** Queue an indication. Only one indication per connection awaits confirmation at a time. */
    TxStatus indicate(const Connection& connection, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                      const ByteArray& value);
    void disconnect(const Connection& connection);
    /**
     * Ask the central for an interval in this range. If it grants one,
     * on_connection_interval_changed() follows once the update takes effect.
     */
    void request_connection_interval(const Connection& connection, std::chrono::microseconds min,
                                     std::chrono::microseconds max);

    /** Call on_timer(id) every period while the environment is active. Restarting an id replaces it. */
    void start_timer(int id, std::chrono::milliseconds period);
    void stop_timer(int id);

    // ---- Events ---------------------------------------------------------------------------------------------------
    //
    // Called by the simulation on this device's executor. Override the ones the device needs; the defaults serve
    // stored values. A write whose handler returns SUCCESS stores the value afterwards, so handlers still see the
    // previous value through value(). UUIDs passed to events are lowercase.

    virtual void on_connected(Connection connection);
    virtual void on_disconnected(Connection connection);
    virtual void on_mtu_changed(Connection connection, uint16_t mtu);
    virtual void on_connection_interval_changed(Connection connection, std::chrono::microseconds interval);

    virtual ReadResult on_read(Connection connection, const BluetoothUUID& service,
                               const BluetoothUUID& characteristic);
    virtual AttStatus on_write_request(Connection connection, const BluetoothUUID& service,
                                       const BluetoothUUID& characteristic, const ByteArray& value);
    virtual void on_write_command(Connection connection, const BluetoothUUID& service,
                                  const BluetoothUUID& characteristic, const ByteArray& value);

    virtual void on_subscribed(Connection connection, const BluetoothUUID& service,
                               const BluetoothUUID& characteristic, SubscriptionKind kind);
    virtual void on_unsubscribed(Connection connection, const BluetoothUUID& service,
                                 const BluetoothUUID& characteristic);
    virtual void on_indication_confirmed(Connection connection, const BluetoothUUID& service,
                                         const BluetoothUUID& characteristic);
    /** TX buffers were freed on this connection. */
    virtual void on_tx_complete(Connection connection);

    virtual ReadResult on_read_descriptor(Connection connection, const BluetoothUUID& service,
                                          const BluetoothUUID& characteristic, const BluetoothUUID& descriptor);
    virtual AttStatus on_write_descriptor(Connection connection, const BluetoothUUID& service,
                                          const BluetoothUUID& characteristic, const BluetoothUUID& descriptor,
                                          const ByteArray& value);

    virtual void on_timer(int id);

    std::shared_ptr<Internal::DeviceCore> internal() const;

  private:
    TxStatus send(const Connection& connection, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                  const ByteArray& value, SubscriptionKind kind);

    std::shared_ptr<Internal::DeviceCore> internal_;
};

}  // namespace SimpleBLE::Simulation
