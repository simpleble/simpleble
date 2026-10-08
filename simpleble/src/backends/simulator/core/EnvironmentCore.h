#pragma once

#include <simpleble/Types.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <kvn_scheduler.hpp>
#include <kvn_threadrunner.hpp>

#include "ConnectionCore.h"
#include "Pdu.h"
#include "SimulatorUtils.h"

namespace SimpleBLE {
class AdapterSimulator;
class PeripheralSimulator;
}  // namespace SimpleBLE

namespace SimpleBLE::Simulation {
class Device;
}

namespace SimpleBLE::Simulation::Internal {

class DeviceCore;
class LinkCore;

/** What one connection event hands over once it ends. */
struct EventContext {
    Actions actions;
    /** PDUs the hosts produced during the event; they go out from the next event. */
    std::vector<Pdu> to_device;
    std::vector<Pdu> to_central;
    size_t tx_freed = 0;
};

struct ConnectAttempt {
    std::mutex mutex;
    bool cancelled = false;
    std::shared_ptr<ConnectionCore> connection;
    std::weak_ptr<SimpleBLE::AdapterSimulator> adapter;
    std::weak_ptr<SimpleBLE::PeripheralSimulator> central;
    /** Moved into the last discovery request when the link is established. */
    std::shared_ptr<std::promise<std::shared_ptr<ConnectionCore>>> ready;
};

class EnvironmentCore : public std::enable_shared_from_this<EnvironmentCore> {
  public:
    EnvironmentCore();
    ~EnvironmentCore();

    static std::shared_ptr<EnvironmentCore> active();

    void activate(bool publish = true);
    void deactivate();
    bool is_active() const;
    void shutdown();

    std::shared_ptr<SimpleBLE::AdapterSimulator> add_adapter(const std::string& identifier,
                                                             const BluetoothAddress& address);
    std::vector<std::shared_ptr<SimpleBLE::AdapterSimulator>> adapters() const;
    void add_device(std::shared_ptr<Device> device);
    void remove_device(const std::shared_ptr<Device>& device);

    /** The radio link between an adapter and a device, created on first use. */
    std::shared_ptr<LinkCore> link(const SimpleBLE::AdapterSimulator* adapter, const DeviceCore* device);

    std::shared_future<std::shared_ptr<ConnectionCore>> request_connection(std::shared_ptr<DeviceCore> device,
                                                                           std::shared_ptr<ConnectAttempt> attempt);
    void cancel_connection(const std::shared_ptr<ConnectAttempt>& attempt);

    void start_timer(const std::shared_ptr<DeviceCore>& device, int id);

    /** Closes the connection at its next connection event. */
    void terminate(const std::shared_ptr<ConnectionCore>& connection, bool from_device);
    /** Closes the connection now. */
    void close(const std::shared_ptr<ConnectionCore>& connection);

  private:
    void start_device(const std::shared_ptr<DeviceCore>& device);
    void advertising_event(std::shared_ptr<DeviceCore> device, uint64_t activation, uint64_t generation);
    /** Connects the first waiting central that heard the advertisement. */
    bool answer_connection_request(const std::shared_ptr<DeviceCore>& device);
    bool establish(const std::shared_ptr<DeviceCore>& device, const std::shared_ptr<ConnectAttempt>& attempt,
                   const std::shared_ptr<SimpleBLE::AdapterSimulator>& adapter);
    /** Whether the adapter receives a packet from the device, and at which RSSI. Air thread only. */
    bool receives(const SimpleBLE::AdapterSimulator* adapter, const DeviceCore* device, int16_t& rssi);

    void timer_event(std::shared_ptr<DeviceCore> device, int id, uint64_t activation, uint64_t generation);
    /** The timer's period, or nothing if it was stopped, restarted, or the environment deactivated. */
    std::optional<Clock::duration> timer_period(const std::shared_ptr<DeviceCore>& device, int id, uint64_t activation,
                                                uint64_t generation) const;

    void connection_event(std::shared_ptr<ConnectionCore> connection);
    /** Runs the packet exchanges of one connection event and returns how long they took. */
    Clock::duration exchange_packets(const std::shared_ptr<ConnectionCore>& connection, Clock::time_point anchor,
                                     EventContext& context);
    /** How long the connection event may last. The caller holds the connection's mutex. */
    Clock::duration event_window(const ConnectionCore& connection) const;

    void deliver_to_device(const std::shared_ptr<ConnectionCore>& connection, Pdu pdu, EventContext& context);
    void deliver_to_central(const std::shared_ptr<ConnectionCore>& connection, Pdu pdu, EventContext& context);
    /** Runs a GATT request through the device's handlers and queues the response. */
    void handle_request(const std::shared_ptr<ConnectionCore>& connection, Pdu pdu);
    void close_locked(const std::shared_ptr<ConnectionCore>& connection, Actions& actions);
    void forget(const std::shared_ptr<ConnectionCore>& connection);

    /** Runs an event on the device's executor. */
    void post_to_device(const std::shared_ptr<DeviceCore>& device, std::function<void(Device&)> event);

    bool current_activation(uint64_t activation) const;

    mutable std::mutex mutex_;
    bool active_ = false;
    uint64_t activation_ = 0;
    std::vector<std::shared_ptr<SimpleBLE::AdapterSimulator>> adapters_;
    std::vector<std::shared_ptr<Device>> devices_;
    std::vector<std::shared_ptr<ConnectionCore>> connections_;
    std::map<DeviceCore*, std::deque<std::shared_ptr<ConnectAttempt>>> pending_;
    std::map<std::pair<const void*, const void*>, std::shared_ptr<LinkCore>> links_;
    std::mt19937 rng_{0};

    kvn::scheduler air_;
    kvn::thread_runner device_executor_;
};

}  // namespace SimpleBLE::Simulation::Internal
