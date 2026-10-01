#pragma once

#include <simpleble/Types.h>

#include <cstddef>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <utility>

#include "Pdu.h"
#include "SimulatorUtils.h"

namespace kvn {
class thread_runner;
}

namespace SimpleBLE {
class AdapterSimulator;
class PeripheralSimulator;
}  // namespace SimpleBLE

namespace SimpleBLE::Simulation::Internal {

class DeviceCore;
class LinkCore;

struct PendingUpdate {
    Clock::duration interval;
    uint64_t instant;
};

class ConnectionCore {
  public:
    std::shared_ptr<DeviceCore> device;
    std::weak_ptr<SimpleBLE::AdapterSimulator> adapter;
    std::weak_ptr<SimpleBLE::PeripheralSimulator> central;
    std::shared_ptr<kvn::thread_runner> central_executor;
    std::shared_ptr<LinkCore> radio;

    mutable std::mutex mutex;
    std::condition_variable transaction_cv;
    bool open = true;
    uint16_t mtu = 23;
    uint16_t client_mtu = 23;
    uint16_t server_mtu = 23;
    size_t data_length = 27;
    size_t tx_buffers = 4;
    Clock::duration interval{};
    Clock::duration supervision_timeout{};
    Clock::time_point anchor{};
    Clock::time_point heard_by_central{};
    Clock::time_point heard_by_device{};
    uint64_t event_counter = 0;
    std::optional<PendingUpdate> pending_update;
    size_t tx_in_use = 0;
    bool request_in_flight = false;
    bool indication_pending = false;
    std::set<std::pair<BluetoothUUID, BluetoothUUID>> notify_subscriptions;
    std::set<std::pair<BluetoothUUID, BluetoothUUID>> indicate_subscriptions;
    std::deque<Pdu> to_device;
    std::deque<Pdu> to_central;

    std::promise<void> closed_promise;
    std::shared_future<void> closed = closed_promise.get_future().share();

    bool is_open() const;

    /** Queues a PDU from the central. Returns false if the connection is closed. */
    bool send_to_device(Pdu pdu);
    /** Queues a PDU from the device. Returns false if the connection is closed. */
    bool send_to_central(Pdu pdu);
    /** Queue a PDU from the central. The caller holds `mutex` and has checked `open`. */
    void enqueue_to_device_locked(Pdu pdu);
    /** Queues a PDU from the device. The caller holds `mutex` and has checked `open`. */
    void enqueue_to_central_locked(Pdu pdu);
};

}  // namespace SimpleBLE::Simulation::Internal
