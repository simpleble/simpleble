#include <gtest/gtest.h>

#include "backends/plain/PeripheralPlain.h"

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

namespace {

const SimpleBLE::BluetoothUUID SERVICE = "0000fff0-0000-1000-8000-00805f9b34fb";
const SimpleBLE::BluetoothUUID CHARACTERISTIC = "0000fff1-0000-1000-8000-00805f9b34fb";

template <typename Condition>
bool eventually(Condition condition, std::chrono::milliseconds timeout) {
    auto deadline = Clock::now() + timeout;
    while (Clock::now() < deadline) {
        if (condition()) return true;
        std::this_thread::sleep_for(10ms);
    }
    return condition();
}

}  // namespace

TEST(PeripheralPlain, NotifiesUntilUnsubscribed) {
    std::atomic<int> received{0};
    SimpleBLE::PeripheralPlain peripheral;
    peripheral.notify(SERVICE, CHARACTERISTIC, [&](SimpleBLE::ByteArray) { received++; });

    // Notifications repeat every second.
    EXPECT_TRUE(eventually([&]() { return received.load() >= 2; }, 4s));

    peripheral.unsubscribe(SERVICE, CHARACTERISTIC);
    int after_unsubscribe = received.load();
    std::this_thread::sleep_for(1500ms);
    EXPECT_EQ(received.load(), after_unsubscribe);
}

TEST(PeripheralPlain, DestroyedDuringNotification) {
    std::promise<void> started;
    auto notification_started = started.get_future();
    std::atomic<bool> first{true};

    auto peripheral = std::make_shared<SimpleBLE::PeripheralPlain>();
    peripheral->notify(SERVICE, CHARACTERISTIC, [&](SimpleBLE::ByteArray) {
        if (first.exchange(false)) started.set_value();
        std::this_thread::sleep_for(50ms);
    });
    ASSERT_EQ(notification_started.wait_for(3s), std::future_status::ready);

    // Destroyed while the notification callback is still running.
    peripheral.reset();
}
