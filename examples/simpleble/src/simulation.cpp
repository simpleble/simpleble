#include <simpleble/Backend.h>
#include <simpleble/SimpleBLE.h>
#include <simpleble/Simulation.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>

using namespace std::chrono_literals;
namespace sim = SimpleBLE::Simulation;

const SimpleBLE::BluetoothUUID HEART_RATE_SERVICE = "0000180d-0000-1000-8000-00805f9b34fb";
const SimpleBLE::BluetoothUUID HEART_RATE_MEASUREMENT = "00002a37-0000-1000-8000-00805f9b34fb";
const SimpleBLE::BluetoothUUID BODY_SENSOR_LOCATION = "00002a38-0000-1000-8000-00805f9b34fb";

const auto START = std::chrono::steady_clock::now();
std::mutex log_mutex;

// The device, SimpleBLE callbacks and main() log from different threads, so each line is printed whole.
template <typename... Parts>
void log(const Parts&... parts) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - START);
    std::ostringstream line;
    line << "[" << std::setw(5) << elapsed.count() << " ms] ";
    (line << ... << parts);

    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << line.str() << std::endl;
}

template <typename Operation>
long long milliseconds_taken(Operation operation) {
    auto begin = std::chrono::steady_clock::now();
    operation();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
}

/**
 * A heart rate monitor that sends a measurement every 250 ms while a central is subscribed.
 */
class HeartRateMonitor : public sim::Device {
  public:
    HeartRateMonitor() : sim::Device("Simulated HRM", "C0:FF:EE:00:00:01") {
        set_advertising_interval(100ms);
        add_advertised_service(HEART_RATE_SERVICE);
        // The adapter connects at its own interval; the device then asks for this one.
        set_preferred_connection_interval(15ms, 15ms);

        add_service(HEART_RATE_SERVICE);
        add_characteristic(HEART_RATE_SERVICE, HEART_RATE_MEASUREMENT, {sim::Property::NOTIFY});
        add_characteristic(HEART_RATE_SERVICE, BODY_SENSOR_LOCATION, {sim::Property::READ});
        set_value(HEART_RATE_SERVICE, BODY_SENSOR_LOCATION, SimpleBLE::ByteArray{0x01});  // Chest
    }

    // Events for one device run one at a time, so members need no locking.

    void on_connection_interval_changed(sim::Connection connection, std::chrono::microseconds interval) override {
        log("[device] Connection interval is now ", interval.count() / 1000.0f, " ms");
    }

    void on_subscribed(sim::Connection connection, const SimpleBLE::BluetoothUUID& service,
                       const SimpleBLE::BluetoothUUID& characteristic, sim::SubscriptionKind kind) override {
        if (characteristic != HEART_RATE_MEASUREMENT) return;
        subscriber_ = connection;
        start_timer(MEASUREMENT_TIMER, 250ms);
    }

    void on_unsubscribed(sim::Connection connection, const SimpleBLE::BluetoothUUID& service,
                         const SimpleBLE::BluetoothUUID& characteristic) override {
        if (characteristic != HEART_RATE_MEASUREMENT) return;
        stop_timer(MEASUREMENT_TIMER);
    }

    void on_disconnected(sim::Connection connection) override {
        if (connection != subscriber_) return;
        stop_timer(MEASUREMENT_TIMER);
    }

    // `id` is the one passed to start_timer().
    void on_timer(int id) override {
        if (id != MEASUREMENT_TIMER) return;
        uint8_t bpm = static_cast<uint8_t>(60 + (beats_++ % 20));
        notify(subscriber_, HEART_RATE_SERVICE, HEART_RATE_MEASUREMENT, SimpleBLE::ByteArray{0x00, bpm});
    }

  private:
    static constexpr int MEASUREMENT_TIMER = 1;
    sim::Connection subscriber_;
    int beats_ = 0;
};

int main() {
    sim::Environment environment;
    environment.add_adapter("sim0", "00:11:22:33:44:55");
    environment.add_device<HeartRateMonitor>();
    environment.activate();

    // From here on, only the regular SimpleBLE API is used.
    std::optional<SimpleBLE::Adapter> adapter;
    for (auto& backend : SimpleBLE::Backend::get_backends()) {
        if (backend.identifier() != "Simulator") continue;
        adapter = backend.adapters().at(0);
    }
    if (!adapter) {
        std::cerr << "The Simulator backend is not active." << std::endl;
        return EXIT_FAILURE;
    }

    std::optional<SimpleBLE::Peripheral> monitor;
    adapter->set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral) {
        log("Found ", peripheral.identifier(), " [", peripheral.address(), "] ", peripheral.rssi(), " dBm");
        if (peripheral.identifier() != "Simulated HRM") return;
        monitor = peripheral;
    });
    adapter->scan_for(300);
    if (!monitor) {
        std::cerr << "The heart rate monitor was not found." << std::endl;
        return EXIT_FAILURE;
    }

    auto connect_ms = milliseconds_taken([&]() { monitor->connect(); });
    log("Connected in ", connect_ms, " ms, MTU ", monitor->mtu());

    for (int i = 0; i < 3; i++) {
        SimpleBLE::ByteArray location;
        auto read_ms = milliseconds_taken([&]() { location = monitor->read(HEART_RATE_SERVICE, BODY_SENSOR_LOCATION); });
        log("Read body sensor location ", int(location[0]), " in ", read_ms, " ms");
        std::this_thread::sleep_for(100ms);
    }

    auto subscribe_ms = milliseconds_taken([&]() {
        monitor->notify(HEART_RATE_SERVICE, HEART_RATE_MEASUREMENT, [&](SimpleBLE::ByteArray payload) {
            log("Heart rate: ", int(payload[1]), " bpm");
        });
    });
    log("Subscribed in ", subscribe_ms, " ms");

    std::this_thread::sleep_for(1s);

    monitor->unsubscribe(HEART_RATE_SERVICE, HEART_RATE_MEASUREMENT);
    auto disconnect_ms = milliseconds_taken([&]() { monitor->disconnect(); });
    log("Disconnected in ", disconnect_ms, " ms");
    return EXIT_SUCCESS;
}
