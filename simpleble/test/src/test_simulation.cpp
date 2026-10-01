#include <gtest/gtest.h>

#include <simpleble/Backend.h>
#include <simpleble/Exceptions.h>
#include <simpleble/SimpleBLE.h>
#include <simpleble/Simulation.h>

#include "backends/simulator/AdapterSimulator.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
namespace sim = SimpleBLE::Simulation;

using SimpleBLE::BluetoothUUID;
using SimpleBLE::ByteArray;

namespace {

const BluetoothUUID SERVICE = "0000aaaa-0000-1000-8000-00805f9b34fb";
const BluetoothUUID CONTROL = "0000aaa1-0000-1000-8000-00805f9b34fb";
const BluetoothUUID DATA = "0000aaa2-0000-1000-8000-00805f9b34fb";
const BluetoothUUID USER_DESCRIPTION = "00002901-0000-1000-8000-00805f9b34fb";
const BluetoothUUID CCCD = "00002902-0000-1000-8000-00805f9b34fb";

class Widget : public sim::Device {
  public:
    Widget() : sim::Device("Widget", "C0:FF:EE:00:00:01") {
        set_advertising_interval(20ms);
        set_manufacturer_data(0xFFFF, {0x01, 0x02});
        add_advertised_service(SERVICE);

        add_service(SERVICE);
        add_characteristic(SERVICE, CONTROL,
                           {sim::Property::READ, sim::Property::WRITE_REQUEST, sim::Property::WRITE_COMMAND});
        add_characteristic(SERVICE, DATA, {sim::Property::NOTIFY, sim::Property::INDICATE});
        add_descriptor(SERVICE, CONTROL, USER_DESCRIPTION);
        set_value(SERVICE, CONTROL, ByteArray{0x00});
        set_value(SERVICE, CONTROL, USER_DESCRIPTION, "Control");
    }

    sim::AttStatus on_write_request(sim::Connection, const BluetoothUUID&, const BluetoothUUID& characteristic,
                                    const ByteArray& value) override {
        if (characteristic == CONTROL && value.size() != 1) return sim::AttStatus::INVALID_ATTRIBUTE_VALUE_LENGTH;
        return sim::AttStatus::SUCCESS;
    }

    void on_write_command(sim::Connection, const BluetoothUUID&, const BluetoothUUID&, const ByteArray&) override {
        commands++;
    }

    void on_subscribed(sim::Connection, const BluetoothUUID&, const BluetoothUUID&, sim::SubscriptionKind) override {
        subscribed = true;
    }

    void on_unsubscribed(sim::Connection, const BluetoothUUID&, const BluetoothUUID&) override { subscribed = false; }

    void on_indication_confirmed(sim::Connection, const BluetoothUUID&, const BluetoothUUID&) override {
        confirmations++;
    }

    void on_tx_complete(sim::Connection) override { tx_completions++; }

    void on_disconnected(sim::Connection) override { disconnections++; }

    void on_connection_interval_changed(sim::Connection, std::chrono::microseconds new_interval) override {
        interval_us = new_interval.count();
        interval_changes++;
    }

    void on_timer(int id) override {
        if (id == 7) {
            ticks++;
            late_ticks += stopped ? 1 : 0;
            return;
        }
        if (id != 8) return;

        // Hold the device's thread so ticks of timer 7 queue up behind this event, then stop it.
        std::this_thread::sleep_for(60ms);
        stop_timer(7);
        stop_timer(8);
        stopped = true;
    }

    std::atomic<int> commands{0};
    std::atomic<bool> subscribed{false};
    std::atomic<int> confirmations{0};
    std::atomic<int> tx_completions{0};
    std::atomic<int> disconnections{0};
    std::atomic<int> ticks{0};
    std::atomic<bool> stopped{false};
    std::atomic<int> late_ticks{0};
    std::atomic<long long> interval_us{0};
    std::atomic<int> interval_changes{0};
};

template <typename Condition>
bool eventually(Condition condition, std::chrono::milliseconds timeout = 3s) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (condition()) return true;
        std::this_thread::sleep_for(5ms);
    }
    return condition();
}

bool simulator_active() {
    for (auto& backend : SimpleBLE::Backend::get_backends()) {
        if (backend.identifier() == "Simulator") return true;
    }
    return false;
}

SimpleBLE::Adapter simulator_adapter(const std::string& identifier = "sim0") {
    for (auto& backend : SimpleBLE::Backend::get_backends()) {
        if (backend.identifier() != "Simulator") continue;
        for (auto& adapter : backend.adapters()) {
            if (adapter.identifier() == identifier) return adapter;
        }
    }
    throw std::runtime_error("No simulated adapter named " + identifier);
}

class SimulationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        environment = std::make_unique<sim::Environment>();
        host = environment->add_adapter("sim0", "00:11:22:33:44:55");
        host.set_connection_interval(15ms);
        widget = environment->add_device<Widget>();
        environment->activate();
        adapter = simulator_adapter();
    }

    void TearDown() override { environment.reset(); }

    SimpleBLE::Peripheral find_widget() { return find_widget(adapter); }

    SimpleBLE::Peripheral find_widget(SimpleBLE::Adapter& scanner) {
        scanner.scan_for(150);
        for (auto& peripheral : scanner.scan_get_results()) {
            if (peripheral.identifier() == "Widget") return peripheral;
        }
        throw std::runtime_error("Widget was not found");
    }

    SimpleBLE::Peripheral connect_widget() {
        auto peripheral = find_widget();
        peripheral.connect();
        return peripheral;
    }

    sim::Connection widget_connection() {
        auto connections = widget->connections();
        if (connections.size() != 1) throw std::runtime_error("Expected one connection");
        return connections.front();
    }

    std::unique_ptr<sim::Environment> environment;
    sim::Adapter host;
    std::shared_ptr<Widget> widget;
    SimpleBLE::Adapter adapter;
};

// A device with a large GATT database, so discovery takes many connection events.
class Catalog : public sim::Device {
  public:
    static constexpr int CHARACTERISTICS = 20;

    Catalog() : sim::Device("Catalog", "C0:FF:EE:00:00:02") {
        set_advertising_interval(20ms);
        set_preferred_connection_interval(15ms, 15ms);
        add_service(SERVICE);
        for (int i = 0; i < CHARACTERISTICS; i++) {
            char uuid[37];
            std::snprintf(uuid, sizeof(uuid), "0000b0%02x-0000-1000-8000-00805f9b34fb", i);
            add_characteristic(SERVICE, uuid, {sim::Property::READ});
        }
    }

    void on_connection_interval_changed(sim::Connection, std::chrono::microseconds) override {
        std::lock_guard<std::mutex> lock(mutex);
        interval_changed_at = std::chrono::steady_clock::now();
    }

    std::mutex mutex;
    std::optional<std::chrono::steady_clock::time_point> interval_changed_at;
};

// Groups arrival times into bursts, one per connection event.
std::vector<size_t> bursts(const std::vector<std::chrono::steady_clock::time_point>& arrivals) {
    std::vector<size_t> sizes;
    for (size_t i = 0; i < arrivals.size(); i++) {
        if (i == 0 || arrivals[i] - arrivals[i - 1] > 40ms) {
            sizes.push_back(0);
        }
        sizes.back()++;
    }
    return sizes;
}

}  // namespace

TEST(Simulation, BackendFollowsEnvironmentActivation) {
    EXPECT_FALSE(simulator_active());
    {
        sim::Environment environment;
        environment.add_adapter("sim0", "00:11:22:33:44:55");
        EXPECT_FALSE(simulator_active());

        environment.activate();
        EXPECT_TRUE(simulator_active());
        EXPECT_EQ(simulator_adapter().identifier(), "sim0");

        sim::Environment other;
        EXPECT_THROW(other.activate(), SimpleBLE::Exception::OperationFailed);

        environment.deactivate();
        EXPECT_FALSE(simulator_active());
        other.activate();
        EXPECT_TRUE(simulator_active());
    }
    EXPECT_FALSE(simulator_active());
}

TEST(Simulation, AdapterCallbacksSurviveEnvironmentDestruction) {
    auto environment = std::make_unique<sim::Environment>();
    auto host = environment->add_adapter("sim0", "00:11:22:33:44:55");
    environment->activate();
    auto adapter = simulator_adapter();
    auto executor = SimpleBLE::AdapterSimulator::from(host.internal()).executor();
    std::weak_ptr<SimpleBLE::AdapterBase> weak_adapter = host.internal();
    std::promise<void> entered, release;
    auto gate = release.get_future().share();
    std::atomic<bool> alive_after_destruction{false};
    std::atomic<int> callbacks{0};
    adapter.set_callback_on_scan_start([&]() {
        entered.set_value();
        gate.wait();
        auto weak = weak_adapter;
        auto* alive = &alive_after_destruction;
        host = sim::Adapter{};
        adapter = SimpleBLE::Adapter{};
        environment.reset();
        *alive = !weak.expired();
    });
    auto count = [&]() { callbacks++; };
    adapter.set_callback_on_scan_stop(count);
    adapter.set_callback_on_power_off(count);
    adapter.set_callback_on_power_on(count);
    adapter.scan_start();
    EXPECT_EQ(entered.get_future().wait_for(2s), std::future_status::ready);
    adapter.scan_stop();
    adapter.power_off();
    adapter.power_on();
    release.set_value();
    executor->stop();

    EXPECT_TRUE(alive_after_destruction.load());
    EXPECT_EQ(callbacks.load(), 3);
    EXPECT_TRUE(weak_adapter.expired());
}

TEST_F(SimulationTest, ScanReportsAdvertisement) {
    std::atomic<int> updates{0};
    adapter.set_callback_on_scan_updated([&](SimpleBLE::Peripheral) { updates++; });
    environment->link(host, widget).set_rssi(-75);

    auto peripheral = find_widget();
    EXPECT_EQ(peripheral.address(), "C0:FF:EE:00:00:01");
    EXPECT_EQ(peripheral.rssi(), -75);
    EXPECT_TRUE(peripheral.is_connectable());
    EXPECT_EQ(peripheral.manufacturer_data().at(0xFFFF), (ByteArray{0x01, 0x02}));

    auto services = peripheral.services();
    ASSERT_EQ(services.size(), 1u);
    EXPECT_EQ(services[0].uuid(), SERVICE);

    // 150 ms of scanning at a 20-30 ms advertising interval.
    EXPECT_GE(updates.load(), 3);
}

TEST_F(SimulationTest, ConnectDiscoversServices) {
    std::atomic<bool> connected{false};
    auto peripheral = find_widget();
    peripheral.set_callback_on_connected([&]() { connected = true; });
    peripheral.connect();

    EXPECT_TRUE(peripheral.is_connected());
    EXPECT_EQ(peripheral.mtu(), 244);
    EXPECT_TRUE(eventually([&]() { return connected.load(); }));
    EXPECT_EQ(widget->connections().size(), 1u);

    auto services = peripheral.services();
    ASSERT_EQ(services.size(), 1u);
    auto characteristics = services[0].characteristics();
    ASSERT_EQ(characteristics.size(), 2u);

    EXPECT_EQ(characteristics[0].uuid(), CONTROL);
    EXPECT_TRUE(characteristics[0].can_read());
    EXPECT_TRUE(characteristics[0].can_write_request());
    EXPECT_TRUE(characteristics[0].can_write_command());
    EXPECT_FALSE(characteristics[0].can_notify());

    EXPECT_EQ(characteristics[1].uuid(), DATA);
    EXPECT_TRUE(characteristics[1].can_notify());
    EXPECT_TRUE(characteristics[1].can_indicate());
    auto descriptors = characteristics[1].descriptors();
    ASSERT_EQ(descriptors.size(), 1u);
    EXPECT_EQ(descriptors[0].uuid(), CCCD);
}

TEST_F(SimulationTest, ReadAndWrite) {
    auto peripheral = connect_widget();

    EXPECT_EQ(peripheral.read(SERVICE, CONTROL), ByteArray{0x00});

    peripheral.write_request(SERVICE, CONTROL, ByteArray{0x05});
    EXPECT_EQ(peripheral.read(SERVICE, CONTROL), ByteArray{0x05});
    EXPECT_EQ(widget->value(SERVICE, CONTROL), ByteArray{0x05});

    // The device rejects the write, so the value is not stored.
    EXPECT_THROW(peripheral.write_request(SERVICE, CONTROL, ByteArray{0x01, 0x02}),
                 SimpleBLE::Exception::OperationFailed);
    EXPECT_EQ(widget->value(SERVICE, CONTROL), ByteArray{0x05});

    peripheral.write_command(SERVICE, CONTROL, ByteArray{0x07});
    EXPECT_TRUE(eventually([&]() { return widget->commands.load() == 1; }));
    EXPECT_TRUE(eventually([&]() { return widget->value(SERVICE, CONTROL) == ByteArray{0x07}; }));

    EXPECT_THROW(peripheral.read(SERVICE, DATA), SimpleBLE::Exception::OperationNotSupported);
    EXPECT_THROW(peripheral.read("0000bbbb-0000-1000-8000-00805f9b34fb", CONTROL),
                 SimpleBLE::Exception::ServiceNotFound);
}

TEST_F(SimulationTest, MtuReportsUsablePayloadSize) {
    auto peripheral = find_widget();
    for (uint16_t mtu : {23, 247, 517}) {
        widget->set_max_mtu(mtu);
        peripheral.connect();
        EXPECT_EQ(peripheral.mtu(), mtu - 3);
        EXPECT_EQ(widget_connection().mtu(), mtu);
        peripheral.disconnect();
        EXPECT_EQ(peripheral.mtu(), 0);
        EXPECT_TRUE(eventually([&]() { return widget->connections().empty(); }));
    }
}

TEST_F(SimulationTest, DeactivationCancelsConnectionBeingEstablished) {
    for (bool reactivate : {false, true}) {
        SCOPED_TRACE(reactivate);
        environment->activate();
        auto peripheral = find_widget();
        std::promise<void> held, release;
        auto gate = release.get_future().share();
        auto holding = std::async(std::launch::async, [&]() {
            SimpleBLE::AdapterSimulator::from(host.internal()).update_policy([&](auto&) {
                held.set_value();
                gate.wait();
            });
        });
        held.get_future().wait();
        auto connecting = std::async(std::launch::async, [&]() {
            try {
                peripheral.connect();
                return true;
            } catch (const SimpleBLE::Exception::OperationFailed&) {
                return false;
            }
        });
        // Let the advertising event enter establish(), which waits for the policy lock.
        EXPECT_EQ(connecting.wait_for(150ms), std::future_status::timeout);
        environment->deactivate();
        EXPECT_EQ(connecting.wait_for(500ms), std::future_status::ready);
        if (reactivate) environment->activate();
        release.set_value();
        holding.get();

        EXPECT_FALSE(connecting.get());
        EXPECT_FALSE(peripheral.is_connected());
        EXPECT_TRUE(widget->connections().empty());
        EXPECT_NO_THROW(peripheral.disconnect());
        environment->deactivate();
    }
}

TEST_F(SimulationTest, Descriptors) {
    auto peripheral = connect_widget();

    EXPECT_EQ(peripheral.read(SERVICE, CONTROL, USER_DESCRIPTION), ByteArray("Control"));
    peripheral.write(SERVICE, CONTROL, USER_DESCRIPTION, ByteArray("Ctl"));
    EXPECT_EQ(peripheral.read(SERVICE, CONTROL, USER_DESCRIPTION), ByteArray("Ctl"));
    EXPECT_THROW(peripheral.read(SERVICE, CONTROL, CCCD), SimpleBLE::Exception::DescriptorNotFound);
}

TEST_F(SimulationTest, Notifications) {
    auto peripheral = connect_widget();

    std::mutex mutex;
    std::vector<ByteArray> received;
    peripheral.notify(SERVICE, DATA, [&](ByteArray payload) {
        std::lock_guard<std::mutex> lock(mutex);
        received.push_back(payload);
    });
    EXPECT_TRUE(eventually([&]() { return widget->subscribed.load(); }));
    EXPECT_EQ(peripheral.read(SERVICE, DATA, CCCD), (ByteArray{0x01, 0x00}));

    auto connection = widget_connection();
    EXPECT_EQ(widget->notify(connection, SERVICE, DATA, ByteArray("hello")), sim::TxStatus::QUEUED);
    EXPECT_TRUE(eventually([&]() {
        std::lock_guard<std::mutex> lock(mutex);
        return received.size() == 1 && received[0] == ByteArray("hello");
    }));
    EXPECT_TRUE(eventually([&]() { return widget->tx_completions.load() >= 1; }));

    peripheral.unsubscribe(SERVICE, DATA);
    EXPECT_TRUE(eventually([&]() { return !widget->subscribed.load(); }));
    EXPECT_EQ(widget->notify(connection, SERVICE, DATA, ByteArray("late")), sim::TxStatus::NOT_SUBSCRIBED);
}

TEST_F(SimulationTest, ReconnectDropsQueuedValuesFromPreviousConnection) {
    auto executor = SimpleBLE::AdapterSimulator::from(host.internal()).executor();
    for (bool indication : {false, true}) {
        SCOPED_TRACE(indication);
        auto peripheral = find_widget();
        std::promise<void> entered, release;
        auto gate = release.get_future().share();
        std::atomic<int> connections{0}, received{0}, last_value{0};
        peripheral.set_callback_on_connected([&]() {
            if (++connections == 1) {
                entered.set_value();
                gate.wait();
            }
        });
        peripheral.connect();
        EXPECT_EQ(entered.get_future().wait_for(2s), std::future_status::ready);
        auto subscribe = [&](std::function<void(ByteArray)> callback) {
            if (indication) peripheral.indicate(SERVICE, DATA, std::move(callback));
            else peripheral.notify(SERVICE, DATA, std::move(callback));
        };
        auto send = [&](uint8_t value) {
            auto connection = widget_connection();
            return indication ? widget->indicate(connection, SERVICE, DATA, {value})
                              : widget->notify(connection, SERVICE, DATA, {value});
        };
        subscribe([](ByteArray) {});
        auto completed = widget->tx_completions.load();
        EXPECT_EQ(send(0x11), sim::TxStatus::QUEUED);
        // TX completion follows delivery to the blocked central callback queue.
        EXPECT_TRUE(eventually([&]() { return widget->tx_completions.load() > completed; }));
        peripheral.disconnect();
        peripheral.connect();
        subscribe([&](ByteArray value) {
            last_value = value.at(0);
            received++;
        });
        release.set_value();
        std::promise<void> drained;
        executor->enqueue([&]() { drained.set_value(); });
        EXPECT_EQ(drained.get_future().wait_for(2s), std::future_status::ready);
        EXPECT_EQ(received.load(), 0);

        EXPECT_EQ(send(0x22), sim::TxStatus::QUEUED);
        EXPECT_TRUE(eventually([&]() { return last_value.load() == 0x22; }));
        EXPECT_EQ(received.load(), 1);
        peripheral.disconnect();
        peripheral.set_callback_on_connected(nullptr);
    }
}

TEST_F(SimulationTest, IndicationsWaitForConfirmation) {
    auto peripheral = connect_widget();

    std::atomic<int> received{0};
    peripheral.indicate(SERVICE, DATA, [&](ByteArray) { received++; });
    EXPECT_TRUE(eventually([&]() { return widget->subscribed.load(); }));

    auto connection = widget_connection();
    EXPECT_EQ(widget->indicate(connection, SERVICE, DATA, ByteArray("one")), sim::TxStatus::QUEUED);
    EXPECT_EQ(widget->indicate(connection, SERVICE, DATA, ByteArray("two")), sim::TxStatus::BUSY);

    EXPECT_TRUE(eventually([&]() { return widget->confirmations.load() == 1; }));
    EXPECT_EQ(received.load(), 1);
    EXPECT_EQ(widget->indicate(connection, SERVICE, DATA, ByteArray("two")), sim::TxStatus::QUEUED);
    EXPECT_TRUE(eventually([&]() { return received.load() == 2; }));
}

TEST_F(SimulationTest, TxBuffersApplyBackpressure) {
    // A long interval keeps the queued notifications on the device while the test fills its buffers.
    host.set_connection_interval(200ms);
    auto peripheral = connect_widget();

    std::atomic<int> received{0};
    peripheral.notify(SERVICE, DATA, [&](ByteArray) { received++; });
    EXPECT_TRUE(eventually([&]() { return widget->subscribed.load(); }));

    auto connection = widget_connection();
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(widget->notify(connection, SERVICE, DATA, ByteArray{static_cast<uint8_t>(i)}),
                  sim::TxStatus::QUEUED);
    }
    EXPECT_EQ(widget->notify(connection, SERVICE, DATA, ByteArray{0xFF}), sim::TxStatus::BUSY);

    EXPECT_TRUE(eventually([&]() { return received.load() == 4; }));
    EXPECT_TRUE(eventually([&]() { return widget->tx_completions.load() >= 1; }));
    EXPECT_EQ(widget->notify(connection, SERVICE, DATA, ByteArray{0xFF}), sim::TxStatus::QUEUED);
}

TEST_F(SimulationTest, OperationsTakeConnectionEvents) {
    host.set_connection_interval(100ms);
    auto peripheral = find_widget();

    // MTU exchange plus 4 discovery requests (services, 1 service, 2 characteristics),
    // each answered one connection event after it is sent.
    auto start = std::chrono::steady_clock::now();
    peripheral.connect();
    auto connect_time = std::chrono::steady_clock::now() - start;
    EXPECT_GE(connect_time, 500ms);
    EXPECT_LT(connect_time, 1500ms);

    start = std::chrono::steady_clock::now();
    peripheral.read(SERVICE, CONTROL);
    auto read_time = std::chrono::steady_clock::now() - start;
    EXPECT_GE(read_time, 95ms);
    EXPECT_LT(read_time, 400ms);
}

TEST_F(SimulationTest, DeviceDisconnects) {
    auto peripheral = connect_widget();
    std::atomic<bool> disconnected{false};
    peripheral.set_callback_on_disconnected([&]() { disconnected = true; });

    widget->disconnect(widget_connection());
    EXPECT_TRUE(eventually([&]() { return disconnected.load(); }));
    EXPECT_FALSE(peripheral.is_connected());
    EXPECT_THROW(peripheral.read(SERVICE, CONTROL), SimpleBLE::Exception::NotConnected);
    EXPECT_TRUE(eventually([&]() { return widget->disconnections.load() == 1; }));
    EXPECT_TRUE(widget->connections().empty());

    // The device advertises again once the connection closes.
    peripheral.connect();
    EXPECT_TRUE(peripheral.is_connected());
}

TEST_F(SimulationTest, CentralDisconnects) {
    auto peripheral = connect_widget();
    peripheral.disconnect();
    EXPECT_FALSE(peripheral.is_connected());
    EXPECT_TRUE(eventually([&]() { return widget->disconnections.load() == 1; }));
}

TEST_F(SimulationTest, EnvironmentDestroyedWhileConnected) {
    auto peripheral = connect_widget();
    environment.reset();

    EXPECT_FALSE(simulator_active());
    EXPECT_FALSE(peripheral.is_connected());
    EXPECT_THROW(peripheral.read(SERVICE, CONTROL), SimpleBLE::Exception::NotConnected);
    EXPECT_EQ(widget->disconnections.load(), 1);
}

TEST_F(SimulationTest, Timers) {
    widget->start_timer(7, 20ms);
    EXPECT_TRUE(eventually([&]() { return widget->ticks.load() >= 5; }));
    widget->stop_timer(7);
    std::this_thread::sleep_for(50ms);
    int ticks = widget->ticks.load();
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(widget->ticks.load(), ticks);
}

TEST_F(SimulationTest, StoppedTimerDropsQueuedTicks) {
    widget->start_timer(7, 10ms);
    widget->start_timer(8, 30ms);
    EXPECT_TRUE(eventually([&]() { return widget->stopped.load(); }));
    std::this_thread::sleep_for(50ms);
    EXPECT_GT(widget->ticks.load(), 0);
    EXPECT_EQ(widget->late_ticks.load(), 0);
}

TEST_F(SimulationTest, NegotiatesPreferredInterval) {
    host.set_connection_interval(30ms);
    widget->set_preferred_connection_interval(15ms, 20ms);
    auto peripheral = connect_widget();

    // The central grants the fastest interval both sides accept.
    EXPECT_TRUE(eventually([&]() { return widget->interval_changes.load() == 1; }));
    EXPECT_EQ(widget->interval_us.load(), 15000);
    EXPECT_EQ(widget_connection().interval(), 15ms);

    // A device can also ask for another interval at any time.
    widget->request_connection_interval(widget_connection(), 50ms, 60ms);
    EXPECT_TRUE(eventually([&]() { return widget->interval_changes.load() == 2; }));
    EXPECT_EQ(widget_connection().interval(), 50ms);
}

TEST_F(SimulationTest, CentralRejectsIntervalsOutsideItsPolicy) {
    host.set_connection_interval(30ms);
    host.set_accepted_connection_intervals(30ms, 4s);
    widget->set_preferred_connection_interval(15ms, 20ms);
    auto peripheral = connect_widget();

    // Well past the request and the instant an update would have taken effect.
    std::this_thread::sleep_for(500ms);
    EXPECT_EQ(widget->interval_changes.load(), 0);
    EXPECT_EQ(widget_connection().interval(), 30ms);
}

TEST_F(SimulationTest, IntervalUpdateDoesNotWaitForDiscovery) {
    host.set_connection_interval(30ms);
    auto catalog = environment->add_device<Catalog>();
    adapter.scan_for(150);
    auto results = adapter.scan_get_results();
    auto found = std::find_if(results.begin(), results.end(),
                              [](SimpleBLE::Peripheral& peripheral) { return peripheral.identifier() == "Catalog"; });
    ASSERT_NE(found, results.end());

    found->connect();
    auto connected_at = std::chrono::steady_clock::now();

    // The device asks for 15 ms right after connecting. The central answers while its discovery
    // requests are still queued, so the update takes effect before discovery ends.
    std::lock_guard<std::mutex> lock(catalog->mutex);
    ASSERT_TRUE(catalog->interval_changed_at.has_value());
    EXPECT_LT(*catalog->interval_changed_at, connected_at);
}

TEST_F(SimulationTest, EventLengthBoundsPacketsPerEvent) {
    host.set_connection_interval(100ms);
    widget->set_tx_buffers(32);
    auto peripheral = connect_widget();

    std::mutex mutex;
    std::vector<std::chrono::steady_clock::time_point> arrivals;
    peripheral.notify(SERVICE, DATA, [&](ByteArray) {
        std::lock_guard<std::mutex> lock(mutex);
        arrivals.push_back(std::chrono::steady_clock::now());
    });
    EXPECT_TRUE(eventually([&]() { return widget->subscribed.load(); }));

    auto largest_burst = [&](size_t count) -> size_t {
        {
            std::lock_guard<std::mutex> lock(mutex);
            arrivals.clear();
        }
        auto connection = widget_connection();
        for (size_t i = 0; i < count; i++) {
            // 20 bytes plus the ATT and L2CAP headers fill exactly one 27-byte packet.
            EXPECT_EQ(widget->notify(connection, SERVICE, DATA, ByteArray(20)), sim::TxStatus::QUEUED);
        }
        EXPECT_TRUE(eventually([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            return arrivals.size() == count;
        }));
        std::lock_guard<std::mutex> lock(mutex);
        auto sizes = bursts(arrivals);
        return *std::max_element(sizes.begin(), sizes.end());
    };

    // Each exchange is an empty packet from the adapter and a 27-byte packet from the device:
    // 80 + 150 + 296 + 150 = 676 us. The adapter's 3.75 ms event fits 5 of them.
    EXPECT_EQ(largest_burst(20), 5u);

    // With a longer adapter event, the device's 7.5 ms event length is the limit: 11 exchanges.
    host.set_max_event_length(10ms);
    EXPECT_EQ(largest_burst(22), 11u);
}

TEST_F(SimulationTest, PacketErrorsSlowTransfersDown) {
    widget->set_tx_buffers(32);
    auto peripheral = connect_widget();

    std::atomic<size_t> received{0};
    peripheral.notify(SERVICE, DATA, [&](ByteArray) { received++; });
    EXPECT_TRUE(eventually([&]() { return widget->subscribed.load(); }));

    auto transfer_time = [&](size_t count) {
        received = 0;
        auto start = std::chrono::steady_clock::now();
        auto connection = widget_connection();
        for (size_t i = 0; i < count; i++) {
            EXPECT_EQ(widget->notify(connection, SERVICE, DATA, ByteArray(20)), sim::TxStatus::QUEUED);
        }
        EXPECT_TRUE(eventually([&]() { return received.load() == count; }, 10s));
        return std::chrono::steady_clock::now() - start;
    };

    auto clean = transfer_time(10);
    environment->link(host, widget).set_packet_error_rate(0.8f);
    auto noisy = transfer_time(10);

    // Corrupted packets are sent again, so everything arrives, only later: events carry
    // fewer packets and most end early after two corrupted packets in a row.
    EXPECT_GT(noisy, 3 * clean);
}

TEST_F(SimulationTest, OutOfRangeLosesTheConnection) {
    host.set_supervision_timeout(300ms);
    auto peripheral = connect_widget();
    std::atomic<bool> disconnected{false};
    peripheral.set_callback_on_disconnected([&]() { disconnected = true; });

    auto link = environment->link(host, widget);
    auto start = std::chrono::steady_clock::now();
    link.set_in_range(false);
    EXPECT_TRUE(eventually([&]() { return disconnected.load(); }));
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, 250ms);
    EXPECT_LT(elapsed, 1s);
    EXPECT_TRUE(eventually([&]() { return widget->disconnections.load() == 1; }));

    // Out of range, the device is not seen either. Back in range, it is.
    adapter.scan_for(150);
    EXPECT_TRUE(adapter.scan_get_results().empty());
    link.set_in_range(true);
    EXPECT_NO_THROW(find_widget());
}

TEST_F(SimulationTest, LostConnectionFailsPendingRequest) {
    host.set_supervision_timeout(300ms);
    auto peripheral = connect_widget();
    environment->link(host, widget).set_in_range(false);

    // The request fails when the connection is lost, not at the 30 s ATT timeout.
    auto start = std::chrono::steady_clock::now();
    EXPECT_THROW(peripheral.read(SERVICE, CONTROL), SimpleBLE::Exception::OperationFailed);
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1s);
}

TEST(Simulation, DisconnectFailsRequestWhileDeviceHandlerIsRunning) {
    class BlockedReader : public sim::Device {
      public:
        BlockedReader(std::shared_future<void> release)
            : sim::Device("Blocked reader", "C0:FF:EE:00:00:01"), release(std::move(release)) {
            set_advertising_interval(20ms);
            add_service(SERVICE);
            add_characteristic(SERVICE, CONTROL, {sim::Property::READ});
        }

        sim::ReadResult on_read(sim::Connection, const BluetoothUUID&, const BluetoothUUID&) override {
            started.set_value();
            release.wait();
            return ByteArray{0x00};
        }

        std::promise<void> started;
        std::shared_future<void> release;
    };

    std::promise<void> release;
    sim::Environment environment;
    auto host = environment.add_adapter("sim0", "00:11:22:33:44:55");
    host.set_connection_interval(15ms);
    auto device = environment.add_device<BlockedReader>(release.get_future().share());
    environment.activate();
    auto adapter = simulator_adapter();
    adapter.scan_for(150);
    auto peripheral = adapter.scan_get_results().at(0);
    peripheral.connect();

    auto read = std::async(std::launch::async, [&]() {
        try {
            peripheral.read(SERVICE, CONTROL);
            return false;
        } catch (const SimpleBLE::Exception::OperationFailed&) {
            return true;
        }
    });
    EXPECT_EQ(device->started.get_future().wait_for(2s), std::future_status::ready);
    EXPECT_NO_THROW(peripheral.disconnect());
    EXPECT_EQ(read.wait_for(500ms), std::future_status::ready);
    // Always release the handler, including when the regression fails.
    release.set_value();
    EXPECT_TRUE(read.get());
}

TEST_F(SimulationTest, UnreachableCentralDoesNotBlockOtherConnectionAttempts) {
    environment->add_adapter("sim1", "00:11:22:33:44:66");
    auto nearby_adapter = simulator_adapter("sim1");
    auto distant = find_widget();
    auto nearby = find_widget(nearby_adapter);
    auto radio = environment->link(host, widget);
    radio.set_in_range(false);

    auto connect = [](SimpleBLE::Peripheral& peripheral) {
        try {
            peripheral.connect();
            return true;
        } catch (const SimpleBLE::Exception::OperationFailed&) {
            return false;
        }
    };
    auto distant_attempt = std::async(std::launch::async, [&]() { return connect(distant); });
    EXPECT_EQ(distant_attempt.wait_for(100ms), std::future_status::timeout);
    auto nearby_attempt = std::async(std::launch::async, [&]() { return connect(nearby); });
    EXPECT_EQ(nearby_attempt.wait_for(2s), std::future_status::ready);
    EXPECT_TRUE(nearby.is_connected());

    // The skipped attempt stays pending and can connect after coming back in range.
    EXPECT_NO_THROW(nearby.disconnect());
    radio.set_in_range(true);
    EXPECT_EQ(distant_attempt.wait_for(2s), std::future_status::ready);
    environment->deactivate();
    EXPECT_TRUE(nearby_attempt.get());
    EXPECT_TRUE(distant_attempt.get());
}

TEST_F(SimulationTest, DeviceServesSeveralCentrals) {
    auto laptop_host = environment->add_adapter("sim1", "00:11:22:33:44:66");
    laptop_host.set_connection_interval(45ms);
    widget->set_max_connections(2);

    auto phone = connect_widget();
    auto laptop_adapter = simulator_adapter("sim1");
    auto laptop = find_widget(laptop_adapter);
    laptop.connect();

    // Each link has its own interval, chosen by its own central.
    auto connections = widget->connections();
    ASSERT_EQ(connections.size(), 2u);
    auto phone_link = connections[0].interval() == 15ms ? connections[0] : connections[1];
    auto laptop_link = phone_link == connections[0] ? connections[1] : connections[0];
    EXPECT_EQ(phone_link.interval(), 15ms);
    EXPECT_EQ(laptop_link.interval(), 45ms);

    // Subscriptions are per link: only the phone subscribes.
    std::atomic<int> phone_received{0};
    phone.notify(SERVICE, DATA, [&](ByteArray) { phone_received++; });
    EXPECT_TRUE(eventually([&]() { return widget->subscribed.load(); }));
    EXPECT_EQ(widget->notify(laptop_link, SERVICE, DATA, ByteArray("x")), sim::TxStatus::NOT_SUBSCRIBED);
    EXPECT_EQ(widget->notify(phone_link, SERVICE, DATA, ByteArray("x")), sim::TxStatus::QUEUED);
    EXPECT_TRUE(eventually([&]() { return phone_received.load() == 1; }));

    // One central leaving does not affect the other.
    laptop.disconnect();
    EXPECT_TRUE(phone.is_connected());
    EXPECT_TRUE(eventually([&]() { return widget->connections().size() == 1; }));
    EXPECT_TRUE(phone_link.is_connected());
    EXPECT_FALSE(laptop_link.is_connected());
}

TEST_F(SimulationTest, DeviceAtConnectionLimitIsNotSeen) {
    environment->add_adapter("sim1", "00:11:22:33:44:66");
    auto laptop_adapter = simulator_adapter("sim1");

    // With the default limit of one, a connected device stops advertising.
    auto phone = connect_widget();
    laptop_adapter.scan_for(150);
    EXPECT_TRUE(laptop_adapter.scan_get_results().empty());

    phone.disconnect();
    EXPECT_NO_THROW(find_widget(laptop_adapter));
}
