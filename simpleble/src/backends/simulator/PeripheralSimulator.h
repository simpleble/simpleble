#pragma once

#include <simpleble/Types.h>

#include <simpleble/simulation/Types.h>

#include "core/DeviceCore.h"
#include "core/Pdu.h"
#include "PeripheralBase.h"

#include <kvn_safe_callback.hpp>

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace SimpleBLE::Simulation::Internal {
class ConnectionCore;
class EnvironmentCore;
}  // namespace SimpleBLE::Simulation::Internal

namespace SimpleBLE {

class AdapterSimulator;

/**
 * Central-side view of a simulated device, as seen through one adapter.
 *
 * GATT operations become PDUs on the connection and block until the response
 * arrives over the simulated link.
 */
class PeripheralSimulator : public PeripheralBase, public std::enable_shared_from_this<PeripheralSimulator> {
  public:
    PeripheralSimulator(std::weak_ptr<Simulation::Internal::EnvironmentCore> environment,
                        std::weak_ptr<AdapterSimulator> adapter, std::weak_ptr<Simulation::Internal::DeviceCore> device,
                        Simulation::Internal::Advertisement advertisement);
    virtual ~PeripheralSimulator() = default;

    void* underlying() const override;

    std::string identifier() override;
    BluetoothAddress address() override;
    BluetoothAddressType address_type() override;
    int16_t rssi() override;
    int16_t tx_power() override;
    uint16_t mtu() override;

    void connect() override;
    void disconnect() override;
    bool is_connected() override;
    bool is_connectable() override;
    bool is_paired() override;
    void unpair() override;

    std::vector<std::shared_ptr<ServiceBase>> available_services() override;
    std::vector<std::shared_ptr<ServiceBase>> advertised_services() override;

    std::map<uint16_t, ByteArray> manufacturer_data() override;

    // clang-format off
    ByteArray read(BluetoothUUID const& service, BluetoothUUID const& characteristic) override;
    void write_request(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) override;
    void write_command(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) override;
    void notify(BluetoothUUID const& service, BluetoothUUID const& characteristic, std::function<void(ByteArray payload)> callback) override;
    void indicate(BluetoothUUID const& service, BluetoothUUID const& characteristic, std::function<void(ByteArray payload)> callback) override;
    void unsubscribe(BluetoothUUID const& service, BluetoothUUID const& characteristic) override;

    ByteArray read(BluetoothUUID const& service, BluetoothUUID const& characteristic, BluetoothUUID const& descriptor) override;
    void write(BluetoothUUID const& service, BluetoothUUID const& characteristic, BluetoothUUID const& descriptor, ByteArray const& data) override;
    // clang-format on

    void set_callback_on_connected(std::function<void()> on_connected) override;
    void set_callback_on_disconnected(std::function<void()> on_disconnected) override;

    // Simulation, called on the adapter's executor.
    void update_advertisement(const Simulation::Internal::Advertisement& advertisement);
    void handle_value(const std::shared_ptr<Simulation::Internal::ConnectionCore>& connection,
                      const BluetoothUUID& service, const BluetoothUUID& characteristic, const ByteArray& value);
    void handle_disconnected(const std::shared_ptr<Simulation::Internal::ConnectionCore>& connection);

  private:
    using Key = std::pair<BluetoothUUID, BluetoothUUID>;

    std::shared_ptr<Simulation::Internal::ConnectionCore> connection() const;
    /** Returns the characteristic's properties, or throws if it does not exist. */
    std::set<Simulation::Property> properties(const BluetoothUUID& service, const BluetoothUUID& characteristic) const;
    std::pair<Simulation::AttStatus, ByteArray> transact(Simulation::Internal::Pdu pdu);
    void subscribe(const BluetoothUUID& service, const BluetoothUUID& characteristic,
                   std::function<void(ByteArray payload)> callback, Simulation::SubscriptionKind kind);

    std::weak_ptr<Simulation::Internal::EnvironmentCore> environment_;
    std::weak_ptr<AdapterSimulator> adapter_;
    std::weak_ptr<Simulation::Internal::DeviceCore> device_;

    mutable std::mutex mutex_;
    Simulation::Internal::Advertisement advertisement_;
    std::shared_ptr<Simulation::Internal::ConnectionCore> connection_;
    std::vector<Simulation::Internal::ServiceData> services_;
    std::map<Key, std::function<void(ByteArray payload)>> value_callbacks_;
    bool paired_ = false;

    kvn::safe_callback<void()> callback_on_connected_;
    kvn::safe_callback<void()> callback_on_disconnected_;
};

}  // namespace SimpleBLE
