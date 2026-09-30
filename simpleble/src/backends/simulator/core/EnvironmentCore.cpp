#include "EnvironmentCore.h"

#include <simpleble/Exceptions.h>
#include <simpleble/simulation/Connection.h>
#include <simpleble/simulation/Device.h>

#include <algorithm>

#include <kvn_threadrunner.hpp>

#include "AdapterSimulator.h"
#include "DeviceCore.h"
#include "LinkCore.h"
#include "LoggingInternal.h"
#include "PeripheralSimulator.h"

namespace SimpleBLE::Simulation::Internal {

namespace {

// Inter-frame space between consecutive packets of a connection event.
constexpr Clock::duration T_IFS = std::chrono::microseconds(150);
// A connection update takes effect this many connection events after the device receives it.
constexpr uint64_t UPDATE_INSTANT_OFFSET = 6;
// Declarations carried by each simulated discovery response.
constexpr size_t DISCOVERY_RESPONSE_LENGTH = 20;

std::mutex g_active_mutex;
std::weak_ptr<EnvironmentCore> g_active;

/**
 * Air time of one packet on the LE 1M PHY: preamble (1 byte), access address (4),
 * header (2), payload and CRC (3), at 8 µs per byte.
 */
Clock::duration air_time(size_t payload) { return std::chrono::microseconds(8 * (10 + payload)); }


/** Whether the head of a queue can be sent. A request waits until the previous one is answered. */
bool sendable(const std::deque<Pdu>& queue, bool request_in_flight) {
    if (queue.empty()) return false;
    const Pdu& head = queue.front();
    return !(head.is_request() && !head.started && request_in_flight);
}

void respond(EventContext& context, Pdu request, AttStatus status, ByteArray value) {
    Pdu response{PduType::RESPONSE};
    response.status = status;
    response.value = std::move(value);
    response.on_response = std::move(request.on_response);
    context.to_central.push_back(std::move(response));
}

}  // namespace

EnvironmentCore::EnvironmentCore() = default;

EnvironmentCore::~EnvironmentCore() {
    // Stop the air before the device executor it posts to.
    air_.stop();
    device_executor_.stop();
}

std::shared_ptr<EnvironmentCore> EnvironmentCore::active() {
    std::lock_guard<std::mutex> lock(g_active_mutex);
    return g_active.lock();
}

void EnvironmentCore::activate() {
    {
        std::lock_guard<std::mutex> lock(g_active_mutex);
        auto current = g_active.lock();
        if (current.get() == this) return;
        if (current) throw Exception::OperationFailed("Another simulation environment is already active");
        g_active = weak_from_this();
    }

    std::vector<std::shared_ptr<Device>> devices;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_ = true;
        activation_++;
        devices = devices_;
    }

    for (auto& device : devices) {
        start_device(device->internal());
    }
}

void EnvironmentCore::deactivate() {
    std::vector<std::shared_ptr<ConnectionCore>> connections;
    std::map<DeviceCore*, std::deque<std::shared_ptr<ConnectAttempt>>> pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_) return;
        active_ = false;
        activation_++;
        connections = connections_;
        pending.swap(pending_);
    }

    {
        std::lock_guard<std::mutex> lock(g_active_mutex);
        if (g_active.lock().get() == this) {
            g_active.reset();
        }
    }

    // Break the promises of connection attempts that were never answered.
    for (auto& [device, attempts] : pending) {
        for (auto& attempt : attempts) {
            std::lock_guard<std::mutex> lock(attempt->mutex);
            attempt->ready.reset();
        }
    }

    for (auto& connection : connections) {
        close(connection);
    }
}

bool EnvironmentCore::is_active() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_;
}

void EnvironmentCore::shutdown() {
    deactivate();
    air_.stop();
    device_executor_.stop();
    for (auto& adapter : adapters()) {
        adapter->shutdown();
    }
}

std::shared_ptr<AdapterSimulator> EnvironmentCore::add_adapter(const std::string& identifier,
                                                               const BluetoothAddress& address) {
    auto adapter = std::make_shared<AdapterSimulator>(weak_from_this(), identifier, address);
    std::lock_guard<std::mutex> lock(mutex_);
    adapters_.push_back(adapter);
    return adapter;
}

std::vector<std::shared_ptr<AdapterSimulator>> EnvironmentCore::adapters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return adapters_;
}

void EnvironmentCore::add_device(std::shared_ptr<Device> device) {
    if (!device) throw Exception::OperationFailed("Cannot add a null device");

    auto core = device->internal();
    {
        std::lock_guard<std::mutex> lock(core->mutex);
        if (!core->self.expired()) throw Exception::OperationFailed("The device already belongs to an environment");
        core->self = device;
        core->environment = weak_from_this();
    }

    bool active;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        devices_.push_back(device);
        active = active_;
    }

    if (!active) return;
    start_device(core);
}

std::shared_ptr<LinkCore> EnvironmentCore::link(const AdapterSimulator* adapter, const DeviceCore* device) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& link = links_[{adapter, device}];
    if (!link) {
        link = std::make_shared<LinkCore>();
    }
    return link;
}

std::shared_future<std::shared_ptr<ConnectionCore>> EnvironmentCore::request_connection(
    std::shared_ptr<DeviceCore> device, std::shared_ptr<ConnectAttempt> attempt) {
    attempt->ready = std::make_shared<std::promise<std::shared_ptr<ConnectionCore>>>();
    auto ready = attempt->ready->get_future().share();

    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) {
        // Dropping the promise fails the attempt right away.
        attempt->ready.reset();
        return ready;
    }
    pending_[device.get()].push_back(std::move(attempt));
    return ready;
}

void EnvironmentCore::cancel_connection(const std::shared_ptr<ConnectAttempt>& attempt) {
    std::shared_ptr<ConnectionCore> connection;
    {
        std::lock_guard<std::mutex> lock(attempt->mutex);
        attempt->cancelled = true;
        attempt->ready.reset();
        connection = attempt->connection;
    }
    if (!connection) return;
    terminate(connection, false);
}

void EnvironmentCore::start_timer(const std::shared_ptr<DeviceCore>& device, int id) {
    uint64_t activation;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_) return;
        activation = activation_;
    }

    uint64_t generation;
    Clock::duration period;
    {
        std::lock_guard<std::mutex> lock(device->mutex);
        auto timer = device->timers.find(id);
        if (timer == device->timers.end()) return;
        std::tie(generation, period) = timer->second;
    }

    air_.schedule_after(period, [this, device, id, activation, generation]() {
        timer_event(device, id, activation, generation);
    });
}

void EnvironmentCore::terminate(const std::shared_ptr<ConnectionCore>& connection, bool from_device) {
    Pdu pdu{PduType::TERMINATE};
    if (from_device) {
        connection->send_to_central(std::move(pdu));
    } else {
        connection->send_to_device(std::move(pdu));
    }
}

void EnvironmentCore::close(const std::shared_ptr<ConnectionCore>& connection) {
    Actions actions;
    {
        std::lock_guard<std::mutex> lock(connection->mutex);
        close_locked(connection, actions);
    }
    for (auto& action : actions) {
        action();
    }
}

// ---- Advertising ------------------------------------------------------------------------------------------------

bool EnvironmentCore::current_activation(uint64_t activation) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_ && activation_ == activation;
}

void EnvironmentCore::start_device(const std::shared_ptr<DeviceCore>& device) {
    uint64_t activation;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        activation = activation_;
    }

    uint64_t generation;
    std::vector<int> timer_ids;
    {
        std::lock_guard<std::mutex> lock(device->mutex);
        generation = ++device->advertising_generation;
        for (auto& [id, timer] : device->timers) {
            timer_ids.push_back(id);
        }
    }

    air_.enqueue([this, device, activation, generation]() { advertising_event(device, activation, generation); });
    for (int id : timer_ids) {
        start_timer(device, id);
    }
}

bool EnvironmentCore::receives(const AdapterSimulator* adapter, const DeviceCore* device, int16_t& rssi) {
    auto radio = link(adapter, device);
    float packet_error_rate;
    {
        std::lock_guard<std::mutex> lock(radio->mutex);
        if (!radio->in_range) return false;
        packet_error_rate = radio->packet_error_rate;
        rssi = radio->rssi;
    }
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) >= packet_error_rate;
}

void EnvironmentCore::advertising_event(std::shared_ptr<DeviceCore> device, uint64_t activation,
                                        uint64_t generation) {
    if (!current_activation(activation)) return;

    Advertisement advertisement;
    Clock::duration interval;
    bool advertising;
    {
        std::lock_guard<std::mutex> lock(device->mutex);
        if (device->advertising_generation != generation) return;
        advertisement = device->advertisement;
        interval = device->advertising_interval;
        // The device stops advertising once it has as many connections as it accepts.
        advertising = device->connections.size() < device->settings.max_connections;
    }

    // Advertising events are spaced by the interval plus a random 0-10 ms delay.
    std::uniform_int_distribution<int> delay_us(0, 10000);
    auto next = Clock::now() + interval + std::chrono::microseconds(delay_us(rng_));
    air_.schedule_at(next, [this, device, activation, generation]() {
        advertising_event(device, activation, generation);
    });

    if (!advertising) return;
    if (advertisement.connectable && answer_connection_request(device)) return;

    for (auto& adapter : adapters()) {
        if (!adapter->scan_is_active()) continue;

        Advertisement received = advertisement;
        if (!receives(adapter.get(), device.get(), received.rssi)) continue;

        std::weak_ptr<DeviceCore> weak_device = device;
        adapter->executor()->enqueue([adapter, received, weak_device]() {
            adapter->receive_advertisement(received, weak_device);
        });
    }
}

bool EnvironmentCore::answer_connection_request(const std::shared_ptr<DeviceCore>& device) {
    std::shared_ptr<ConnectAttempt> attempt;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto pending = pending_.find(device.get());
        if (pending == pending_.end()) return false;

        auto& attempts = pending->second;
        while (!attempts.empty() && !attempt) {
            auto candidate = attempts.front();
            std::lock_guard<std::mutex> attempt_lock(candidate->mutex);
            if (candidate->cancelled || candidate->adapter.expired()) {
                attempts.pop_front();
                continue;
            }
            attempt = candidate;
        }
    }
    if (!attempt) return false;

    // The central connects only if it heard this advertisement.
    auto adapter = attempt->adapter.lock();
    int16_t rssi;
    if (!adapter || !receives(adapter.get(), device.get(), rssi)) return false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto pending = pending_.find(device.get());
        // Deactivation may have taken the attempt meanwhile.
        if (pending == pending_.end() || pending->second.empty() || pending->second.front() != attempt) return false;
        pending->second.pop_front();
    }

    return establish(device, attempt, adapter);
}

bool EnvironmentCore::establish(const std::shared_ptr<DeviceCore>& device,
                                const std::shared_ptr<ConnectAttempt>& attempt,
                                const std::shared_ptr<AdapterSimulator>& adapter) {
    auto central = attempt->central.lock();
    if (!central) return false;

    const auto policy = adapter->policy();
    auto connection = std::make_shared<ConnectionCore>();
    connection->device = device;
    connection->adapter = adapter;
    connection->central = central;
    connection->central_executor = adapter->executor();
    connection->radio = link(adapter.get(), device.get());
    connection->client_mtu = policy.mtu;
    connection->interval = policy.connection_interval;
    connection->supervision_timeout = policy.supervision_timeout;

    // Discovery: one request for the primary services, then one per service for its
    // characteristics and one per characteristic for its descriptors.
    size_t discovery_requests = 1;
    std::optional<std::pair<Clock::duration, Clock::duration>> preferred_interval;
    {
        std::lock_guard<std::mutex> lock(device->mutex);
        connection->server_mtu = device->settings.max_mtu;
        connection->data_length = std::min<size_t>(device->settings.max_data_length, policy.max_data_length);
        connection->tx_buffers = device->settings.tx_buffers;
        preferred_interval = device->settings.preferred_interval;
        for (auto& service : device->services) {
            discovery_requests += 1 + service.characteristics.size();
        }
    }

    const auto now = Clock::now();
    connection->anchor = now + connection->interval;
    connection->heard_by_central = now;
    connection->heard_by_device = now;

    std::shared_ptr<std::promise<std::shared_ptr<ConnectionCore>>> ready;
    {
        std::lock_guard<std::mutex> lock(attempt->mutex);
        if (attempt->cancelled) return false;
        attempt->connection = connection;
        ready = std::move(attempt->ready);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.push_back(connection);
    }
    {
        std::lock_guard<std::mutex> lock(device->mutex);
        device->connections.push_back(connection);
    }

    // The device's stack asks for its preferred interval if the central picked another one.
    const bool outside_preference = preferred_interval && (connection->interval < preferred_interval->first ||
                                                           connection->interval > preferred_interval->second);
    if (outside_preference) {
        Pdu request{PduType::CONNECTION_PARAMETER_REQUEST};
        request.interval_min = preferred_interval->first;
        request.interval_max = preferred_interval->second;
        connection->send_to_central(std::move(request));
    }

    connection->send_to_device(Pdu{PduType::MTU_REQUEST});
    for (size_t i = 0; i < discovery_requests; i++) {
        Pdu request{PduType::DISCOVERY_REQUEST};
        // The last discovery response completes the connection.
        if (i + 1 == discovery_requests && ready) {
            request.on_response = [ready, connection](AttStatus, ByteArray) { ready->set_value(connection); };
        }
        connection->send_to_device(std::move(request));
    }

    post_to_device(device, [connection](Device& d) { d.on_connected(Connection(connection)); });
    air_.schedule_at(connection->anchor, [this, connection]() { connection_event(connection); });
    return true;
}

// ---- Timers -----------------------------------------------------------------------------------------------------

void EnvironmentCore::timer_event(std::shared_ptr<DeviceCore> device, int id, uint64_t activation,
                                  uint64_t generation) {
    auto period = timer_period(device, id, activation, generation);
    if (!period) return;

    // Checked again on the device's thread: the timer may be stopped or restarted
    // while this tick waits behind other events of the device.
    post_to_device(device, [this, device, id, activation, generation](Device& d) {
        if (!timer_period(device, id, activation, generation)) return;
        d.on_timer(id);
    });
    air_.schedule_after(*period, [this, device, id, activation, generation]() {
        timer_event(device, id, activation, generation);
    });
}

std::optional<Clock::duration> EnvironmentCore::timer_period(const std::shared_ptr<DeviceCore>& device, int id,
                                                             uint64_t activation, uint64_t generation) const {
    if (!current_activation(activation)) return std::nullopt;

    std::lock_guard<std::mutex> lock(device->mutex);
    auto timer = device->timers.find(id);
    if (timer == device->timers.end() || timer->second.first != generation) return std::nullopt;
    return timer->second.second;
}

// ---- Connection events ------------------------------------------------------------------------------------------

void EnvironmentCore::connection_event(std::shared_ptr<ConnectionCore> connection) {
    EventContext context;
    Clock::time_point anchor;
    Clock::time_point next_anchor;
    Clock::duration elapsed{};
    bool open;
    {
        std::lock_guard<std::mutex> lock(connection->mutex);
        if (!connection->open) return;

        anchor = connection->anchor;
        connection->event_counter++;

        // A connection update takes effect at its instant.
        const bool update_due = connection->pending_update &&
                                connection->event_counter >= connection->pending_update->instant;
        if (update_due) {
            connection->interval = connection->pending_update->interval;
            connection->pending_update.reset();
            auto interval = std::chrono::duration_cast<std::chrono::microseconds>(connection->interval);
            context.actions.push_back([this, connection, interval]() {
                post_to_device(connection->device, [connection, interval](Device& d) {
                    d.on_connection_interval_changed(Connection(connection), interval);
                });
            });
        }

        // The link is lost when either side hears nothing valid for the supervision timeout.
        const auto last_heard = std::min(connection->heard_by_central, connection->heard_by_device);
        if (anchor - last_heard > connection->supervision_timeout) {
            close_locked(connection, context.actions);
        } else {
            elapsed = exchange_packets(connection, anchor, context);
        }

        if (connection->open) {
            // What the hosts produced during this event goes out from the next one.
            for (auto& pdu : context.to_device) {
                connection->enqueue_to_device_locked(std::move(pdu));
            }
            for (auto& pdu : context.to_central) {
                connection->enqueue_to_central_locked(std::move(pdu));
            }
            if (context.tx_freed > 0) {
                context.actions.push_back([this, connection]() {
                    post_to_device(connection->device,
                                   [connection](Device& d) { d.on_tx_complete(Connection(connection)); });
                });
            }
            connection->anchor += connection->interval;
        }

        open = connection->open;
        next_anchor = connection->anchor;
    }

    // What was received reaches the hosts when the event ends.
    if (!context.actions.empty()) {
        auto actions = std::make_shared<Actions>(std::move(context.actions));
        air_.schedule_at(anchor + elapsed, [actions]() {
            for (auto& action : *actions) {
                action();
            }
        });
    }

    if (!open) return;
    air_.schedule_at(next_anchor, [this, connection]() { connection_event(connection); });
}

Clock::duration EnvironmentCore::exchange_packets(const std::shared_ptr<ConnectionCore>& connection,
                                                  Clock::time_point anchor, EventContext& context) {
    float packet_error_rate;
    {
        std::lock_guard<std::mutex> lock(connection->radio->mutex);
        // Out of range, the device never hears the anchor packet and the event is lost.
        if (!connection->radio->in_range) return Clock::duration::zero();
        packet_error_rate = connection->radio->packet_error_rate;
    }

    const Clock::duration window = event_window(*connection);
    std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
    auto corrupted = [&]() { return uniform(rng_) < packet_error_rate; };
    Clock::duration elapsed{};
    int consecutive_errors = 0;

    // The central opens every exchange and the device answers, each sending an empty packet if it has
    // nothing to send. The first exchange always happens; later ones only if they fit in the event.
    for (bool first = true; connection->open; first = false) {
        Pdu* from_central = sendable(connection->to_device, connection->request_in_flight)
                                ? &connection->to_device.front()
                                : nullptr;
        Pdu* from_device = sendable(connection->to_central, false) ? &connection->to_central.front() : nullptr;
        const size_t central_length = from_central ? std::min(from_central->bytes_left, connection->data_length) : 0;
        const size_t device_length = from_device ? std::min(from_device->bytes_left, connection->data_length) : 0;

        const auto exchange = air_time(central_length) + T_IFS + air_time(device_length) + T_IFS;
        if (!first && elapsed + exchange > window) break;
        elapsed += exchange;

        if (from_central && from_central->is_request() && !from_central->started) {
            connection->request_in_flight = true;
        }
        if (from_central) {
            from_central->started = true;
        }

        // Central to device. A corrupted packet is sent again in the next exchange.
        if (corrupted()) {
            consecutive_errors++;
        } else {
            consecutive_errors = 0;
            connection->heard_by_device = anchor + elapsed;
            if (from_central) {
                from_central->bytes_left -= central_length;
            }
            if (from_central && from_central->bytes_left == 0) {
                Pdu pdu = std::move(connection->to_device.front());
                connection->to_device.pop_front();
                deliver_to_device(connection, std::move(pdu), context);
            }
        }
        if (!connection->open) break;

        // Device to central.
        if (corrupted()) {
            consecutive_errors++;
        } else {
            consecutive_errors = 0;
            connection->heard_by_central = anchor + elapsed;
            if (from_device) {
                from_device->bytes_left -= device_length;
            }
            if (from_device && from_device->bytes_left == 0) {
                Pdu pdu = std::move(connection->to_central.front());
                connection->to_central.pop_front();
                if (pdu.uses_tx_buffer) {
                    connection->tx_in_use--;
                    context.tx_freed++;
                }
                deliver_to_central(connection, std::move(pdu), context);
            }
        }
        if (!connection->open) break;

        // Two consecutive CRC errors close the event.
        if (consecutive_errors >= 2) break;
        // Otherwise the event continues while either side has more to send.
        const bool more_data = sendable(connection->to_device, connection->request_in_flight) ||
                               sendable(connection->to_central, false);
        if (!more_data) break;
    }

    return elapsed;
}

Clock::duration EnvironmentCore::event_window(const ConnectionCore& connection) const {
    // The event must end before the next anchor, and within both controllers' event length.
    Clock::duration window = connection.interval - T_IFS;
    if (auto adapter = connection.adapter.lock()) {
        window = std::min(window, adapter->policy().max_event_length);
    }
    std::lock_guard<std::mutex> lock(connection.device->mutex);
    return std::min(window, connection.device->settings.max_event_length);
}

void EnvironmentCore::deliver_to_device(const std::shared_ptr<ConnectionCore>& connection, Pdu pdu,
                                        EventContext& context) {
    auto& device = connection->device;
    const auto key = std::make_pair(pdu.service, pdu.characteristic);

    switch (pdu.type) {
        case PduType::MTU_REQUEST: {
            connection->mtu = std::max<uint16_t>(23, std::min(connection->client_mtu, connection->server_mtu));
            const uint16_t mtu = connection->mtu;
            respond(context, std::move(pdu), AttStatus::SUCCESS, {});
            context.actions.push_back([this, connection, mtu]() {
                post_to_device(connection->device,
                               [connection, mtu](Device& d) { d.on_mtu_changed(Connection(connection), mtu); });
            });
            return;
        }

        case PduType::DISCOVERY_REQUEST:
            respond(context, std::move(pdu), AttStatus::SUCCESS, ByteArray(DISCOVERY_RESPONSE_LENGTH));
            return;

        case PduType::READ:
        case PduType::WRITE_REQUEST:
        case PduType::WRITE_COMMAND: {
            // The ATT server checks permissions before the application sees the request.
            const Property required = pdu.type == PduType::READ            ? Property::READ
                                      : pdu.type == PduType::WRITE_REQUEST ? Property::WRITE_REQUEST
                                                                           : Property::WRITE_COMMAND;
            AttStatus status = AttStatus::SUCCESS;
            {
                std::lock_guard<std::mutex> lock(device->mutex);
                auto* characteristic = device->characteristic(pdu.service, pdu.characteristic);
                if (!characteristic) {
                    status = AttStatus::INVALID_HANDLE;
                } else if (!characteristic->has(required)) {
                    status = pdu.type == PduType::READ ? AttStatus::READ_NOT_PERMITTED : AttStatus::WRITE_NOT_PERMITTED;
                }
            }

            if (status == AttStatus::SUCCESS) {
                context.actions.push_back([this, connection, pdu = std::move(pdu)]() mutable {
                    handle_request(connection, std::move(pdu));
                });
                return;
            }
            // Commands have no response to carry an error.
            if (pdu.type == PduType::WRITE_COMMAND) return;
            respond(context, std::move(pdu), status, {});
            return;
        }

        case PduType::READ_DESCRIPTOR:
        case PduType::WRITE_DESCRIPTOR: {
            bool can_notify = false;
            bool can_indicate = false;
            bool exists = false;
            {
                std::lock_guard<std::mutex> lock(device->mutex);
                auto* characteristic = device->characteristic(pdu.service, pdu.characteristic);
                if (characteristic) {
                    can_notify = characteristic->has(Property::NOTIFY);
                    can_indicate = characteristic->has(Property::INDICATE);
                    exists = pdu.descriptor == CCCD_UUID ? characteristic->subscribable()
                                                         : characteristic->descriptor(pdu.descriptor) != nullptr;
                }
            }

            if (!exists) {
                respond(context, std::move(pdu), AttStatus::INVALID_HANDLE, {});
                return;
            }

            if (pdu.descriptor != CCCD_UUID) {
                context.actions.push_back([this, connection, pdu = std::move(pdu)]() mutable {
                    handle_request(connection, std::move(pdu));
                });
                return;
            }

            // The Client Characteristic Configuration descriptor is handled by the simulated stack.
            const bool notifying = connection->notify_subscriptions.count(key) > 0;
            const bool indicating = connection->indicate_subscriptions.count(key) > 0;

            if (pdu.type == PduType::READ_DESCRIPTOR) {
                ByteArray value = {static_cast<uint8_t>((notifying ? 0x01 : 0x00) | (indicating ? 0x02 : 0x00)), 0x00};
                respond(context, std::move(pdu), AttStatus::SUCCESS, value);
                return;
            }

            if (pdu.value.size() != 2) {
                respond(context, std::move(pdu), AttStatus::INVALID_ATTRIBUTE_VALUE_LENGTH, {});
                return;
            }

            const bool notify = (pdu.value[0] & 0x01) != 0;
            const bool indicate = (pdu.value[0] & 0x02) != 0;
            if ((notify && !can_notify) || (indicate && !can_indicate)) {
                // Client Characteristic Configuration Descriptor Improperly Configured.
                respond(context, std::move(pdu), static_cast<AttStatus>(0xFD), {});
                return;
            }

            if (notify) {
                connection->notify_subscriptions.insert(key);
            } else {
                connection->notify_subscriptions.erase(key);
            }
            if (indicate) {
                connection->indicate_subscriptions.insert(key);
            } else {
                connection->indicate_subscriptions.erase(key);
            }
            respond(context, std::move(pdu), AttStatus::SUCCESS, {});

            const bool was_subscribed = notifying || indicating;
            const bool is_subscribed = notify || indicate;
            const BluetoothUUID service = key.first;
            const BluetoothUUID characteristic = key.second;
            if (!was_subscribed && is_subscribed) {
                const auto kind = indicate ? SubscriptionKind::INDICATE : SubscriptionKind::NOTIFY;
                context.actions.push_back([this, connection, service, characteristic, kind]() {
                    post_to_device(connection->device, [=](Device& d) {
                        d.on_subscribed(Connection(connection), service, characteristic, kind);
                    });
                });
            }
            if (was_subscribed && !is_subscribed) {
                context.actions.push_back([this, connection, service, characteristic]() {
                    post_to_device(connection->device, [=](Device& d) {
                        d.on_unsubscribed(Connection(connection), service, characteristic);
                    });
                });
            }
            return;
        }

        case PduType::CONFIRMATION: {
            connection->indication_pending = false;
            const BluetoothUUID service = key.first;
            const BluetoothUUID characteristic = key.second;
            context.actions.push_back([this, connection, service, characteristic]() {
                post_to_device(connection->device, [=](Device& d) {
                    d.on_indication_confirmed(Connection(connection), service, characteristic);
                });
            });
            return;
        }

        case PduType::CONNECTION_PARAMETER_RESPONSE:
            // A rejection leaves the interval unchanged; an acceptance is followed by the update itself.
            return;

        case PduType::CONNECTION_UPDATE:
            connection->pending_update = PendingUpdate{pdu.interval_min,
                                                       connection->event_counter + UPDATE_INSTANT_OFFSET};
            return;

        case PduType::TERMINATE:
            close_locked(connection, context.actions);
            return;

        default:
            return;
    }
}

void EnvironmentCore::handle_request(const std::shared_ptr<ConnectionCore>& connection, Pdu pdu) {
    post_to_device(connection->device, [connection, pdu = std::move(pdu)](Device& device) mutable {
        Connection handle(connection);
        AttStatus status = AttStatus::SUCCESS;
        ByteArray value;

        try {
            switch (pdu.type) {
                case PduType::READ: {
                    auto result = device.on_read(handle, pdu.service, pdu.characteristic);
                    status = result.status;
                    value = std::move(result.value);
                    break;
                }
                case PduType::WRITE_REQUEST:
                    status = device.on_write_request(handle, pdu.service, pdu.characteristic, pdu.value);
                    if (status == AttStatus::SUCCESS) {
                        device.set_value(pdu.service, pdu.characteristic, pdu.value);
                    }
                    break;
                case PduType::WRITE_COMMAND:
                    device.on_write_command(handle, pdu.service, pdu.characteristic, pdu.value);
                    device.set_value(pdu.service, pdu.characteristic, pdu.value);
                    return;
                case PduType::READ_DESCRIPTOR: {
                    auto result = device.on_read_descriptor(handle, pdu.service, pdu.characteristic, pdu.descriptor);
                    status = result.status;
                    value = std::move(result.value);
                    break;
                }
                case PduType::WRITE_DESCRIPTOR:
                    status = device.on_write_descriptor(handle, pdu.service, pdu.characteristic, pdu.descriptor,
                                                        pdu.value);
                    if (status == AttStatus::SUCCESS) {
                        device.set_value(pdu.service, pdu.characteristic, pdu.descriptor, pdu.value);
                    }
                    break;
                default:
                    return;
            }
        } catch (const std::exception& ex) {
            SIMPLEBLE_LOG_ERROR(fmt::format("Simulated device '{}' failed to handle a request: {}", device.name(),
                                            ex.what()));
            status = AttStatus::UNLIKELY_ERROR;
            value = {};
        }

        Pdu response{PduType::RESPONSE};
        response.status = status;
        response.value = std::move(value);
        response.on_response = std::move(pdu.on_response);
        // Dropped if the connection closed meanwhile, which fails the pending request.
        connection->send_to_central(std::move(response));
    });
}

void EnvironmentCore::deliver_to_central(const std::shared_ptr<ConnectionCore>& connection, Pdu pdu,
                                         EventContext& context) {
    switch (pdu.type) {
        case PduType::RESPONSE: {
            connection->request_in_flight = false;
            if (!pdu.on_response) return;
            context.actions.push_back([handler = std::move(pdu.on_response), status = pdu.status,
                                       value = std::move(pdu.value)]() { handler(status, value); });
            return;
        }

        case PduType::NOTIFICATION:
        case PduType::INDICATION: {
            if (pdu.type == PduType::INDICATION) {
                // The central's stack confirms indications on its own.
                Pdu confirmation{PduType::CONFIRMATION, pdu.service, pdu.characteristic};
                context.to_device.push_back(std::move(confirmation));
            }
            auto central = connection->central;
            auto executor = connection->central_executor;
            context.actions.push_back([central, executor, service = pdu.service, characteristic = pdu.characteristic,
                                       value = std::move(pdu.value)]() {
                executor->enqueue([central, service, characteristic, value]() {
                    auto peripheral = central.lock();
                    if (!peripheral) return;
                    peripheral->handle_value(service, characteristic, value);
                });
            });
            return;
        }

        case PduType::CONNECTION_PARAMETER_REQUEST: {
            // The central grants the fastest interval that both sides accept, or rejects the request.
            CentralPolicy policy;
            if (auto adapter = connection->adapter.lock()) {
                policy = adapter->policy();
            }
            const auto fastest = std::max(pdu.interval_min, policy.accepted_interval_min);
            const auto slowest = std::min(pdu.interval_max, policy.accepted_interval_max);
            const bool accepted = fastest <= slowest;

            // The reply only costs air time: the device does not act on a refusal.
            context.to_device.push_back(Pdu{PduType::CONNECTION_PARAMETER_RESPONSE});

            if (!accepted || fastest == connection->interval) return;
            Pdu update{PduType::CONNECTION_UPDATE};
            update.interval_min = fastest;
            context.to_device.push_back(std::move(update));
            return;
        }

        case PduType::TERMINATE:
            close_locked(connection, context.actions);
            return;

        default:
            return;
    }
}

void EnvironmentCore::close_locked(const std::shared_ptr<ConnectionCore>& connection, Actions& actions) {
    if (!connection->open) return;

    connection->open = false;
    // Dropping the queued requests breaks their promises, which fails the callers waiting on them.
    connection->to_device.clear();
    connection->to_central.clear();
    connection->notify_subscriptions.clear();
    connection->indicate_subscriptions.clear();
    connection->pending_update.reset();
    connection->closed_promise.set_value();

    actions.push_back([this, connection]() {
        forget(connection);
        post_to_device(connection->device, [connection](Device& d) { d.on_disconnected(Connection(connection)); });
        auto central = connection->central;
        connection->central_executor->enqueue([central, connection]() {
            auto peripheral = central.lock();
            if (!peripheral) return;
            peripheral->handle_disconnected(connection);
        });
    });
}

void EnvironmentCore::forget(const std::shared_ptr<ConnectionCore>& connection) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(std::remove(connections_.begin(), connections_.end(), connection), connections_.end());
    }
    auto& device = connection->device;
    std::lock_guard<std::mutex> lock(device->mutex);
    auto& list = device->connections;
    list.erase(std::remove_if(list.begin(), list.end(),
                              [&](const std::weak_ptr<ConnectionCore>& c) {
                                  auto locked = c.lock();
                                  return !locked || locked == connection;
                              }),
               list.end());
}

void EnvironmentCore::post_to_device(const std::shared_ptr<DeviceCore>& device, std::function<void(Device&)> event) {
    device_executor_.enqueue([device, event = std::move(event)]() {
        auto self = device->self.lock();
        if (!self) return;
        try {
            event(*self);
        } catch (const std::exception& ex) {
            SIMPLEBLE_LOG_ERROR(fmt::format("Simulated device '{}' threw from an event: {}", self->name(), ex.what()));
        } catch (...) {
            SIMPLEBLE_LOG_ERROR(fmt::format("Simulated device '{}' threw from an event", self->name()));
        }
    });
}

}  // namespace SimpleBLE::Simulation::Internal
