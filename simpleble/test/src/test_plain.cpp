#include <gtest/gtest.h>

#include <simpleble/Backend.h>
#include <simpleble/Config.h>
#include <simpleble/SimpleBLE.h>
#include <simpleble/Simulation.h>
#include <simpleble/local/Peripheral.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

using namespace SimpleBLE;
using namespace std::chrono_literals;

namespace {

const BluetoothUUID BATTERY_SERVICE = "0000180f-0000-1000-8000-00805f9b34fb";
const BluetoothUUID BATTERY_VALUE = "00002a19-0000-1000-8000-00805f9b34fb";
const BluetoothUUID SERVICE = "0000fff0-0000-1000-8000-00805f9b34fb";
const BluetoothUUID VALUE = "0000fff1-0000-1000-8000-00805f9b34fb";
const BluetoothUUID ERROR_VALUE = "0000fff2-0000-1000-8000-00805f9b34fb";
const BluetoothUUID DESCRIPTION = "00002901-0000-1000-8000-00805f9b34fb";
const BluetoothUUID CCCD = "00002902-0000-1000-8000-00805f9b34fb";

template <typename F>
bool eventually(F condition) {
    auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!condition()) {
        if (std::chrono::steady_clock::now() >= deadline) return false;
        std::this_thread::sleep_for(5ms);
    }
    return true;
}

class Plain : public testing::Test {
  protected:
    void SetUp() override {
        auto adapters = Adapter::get_adapters();
        ASSERT_EQ(adapters.size(), 1u);
        adapter = adapters.front();
        adapter.power_on();
    }

    void TearDown() override {
        adapter.scan_stop();
        for (auto& peripheral : adapter.get_connected_peripherals()) {
            peripheral.set_callback_on_connected(nullptr);
            peripheral.set_callback_on_disconnected(nullptr);
            peripheral.disconnect();
        }

        adapter.set_callback_on_scan_start(nullptr);
        adapter.set_callback_on_scan_stop(nullptr);
        adapter.set_callback_on_scan_found(nullptr);
        adapter.set_callback_on_scan_updated(nullptr);
        adapter.set_callback_on_power_on(nullptr);
        adapter.set_callback_on_power_off(nullptr);
        adapter.power_on();
    }

    Peripheral discover(const std::string& name = "Plain Peripheral") {
        adapter.scan_for(250);
        for (auto& peripheral : adapter.scan_get_results()) {
            if (peripheral.identifier() == name) return peripheral;
        }
        throw std::runtime_error("Expected simulated peripheral was not advertised");
    }

    Adapter adapter;
};

TEST_F(Plain, EnumerationSharesAdapterAndPeripheralState) {
    auto backends = Backend::get_backends();
    ASSERT_EQ(backends.size(), 1u);
    EXPECT_EQ(backends.front().identifier(), "Plain");

    auto copy = backends.front().adapters().front();
    auto found = std::make_shared<std::atomic<int>>(0);
    auto updated = std::make_shared<std::atomic<int>>(0);
    adapter.set_callback_on_scan_found([found](Peripheral) { ++*found; });
    adapter.set_callback_on_scan_updated([updated](Peripheral) { ++*updated; });

    auto peripheral = discover();
    EXPECT_EQ(found->load(), 1);
    EXPECT_GE(updated->load(), 1);
    EXPECT_EQ(copy.scan_get_results().size(), 1u);

    EXPECT_EQ(peripheral.address(), "11:22:33:44:55:66");
    EXPECT_EQ(peripheral.rssi(), -60);
    EXPECT_EQ(peripheral.tx_power(), 5);
    EXPECT_EQ(peripheral.address_type(), BluetoothAddressType::PUBLIC);
    EXPECT_EQ(peripheral.underlying(), nullptr);
    EXPECT_EQ(peripheral.manufacturer_data().at(0x004c), ByteArray("test"));
    ASSERT_EQ(peripheral.services().size(), 2u);
    EXPECT_EQ(peripheral.services().front().data(), (ByteArray{0x00, 0x7f, 0x80, 0xff}));

    peripheral.connect();
    EXPECT_TRUE(copy.scan_get_results().front().is_connected());
    EXPECT_EQ(copy.get_connected_peripherals().size(), 1u);
    EXPECT_EQ(peripheral.mtu(), 244);
    EXPECT_EQ(peripheral.services().size(), 2u);
    EXPECT_EQ(peripheral.read(BATTERY_SERVICE, BATTERY_VALUE), (ByteArray{100}));

    peripheral.disconnect();
}

TEST_F(Plain, PairingSurvivesDisconnectAndCanBeRemoved) {
    auto peripheral = discover();
    peripheral.unpair();
    EXPECT_TRUE(adapter.get_paired_peripherals().empty());

    auto connected = std::make_shared<std::promise<void>>();
    auto disconnected = std::make_shared<std::promise<void>>();
    auto connected_event = connected->get_future();
    auto disconnected_event = disconnected->get_future();
    peripheral.set_callback_on_connected([connected] { connected->set_value(); });
    peripheral.set_callback_on_disconnected([disconnected] { disconnected->set_value(); });

    peripheral.connect();
    EXPECT_EQ(connected_event.wait_for(2s), std::future_status::ready);
    EXPECT_TRUE(peripheral.is_paired());
    EXPECT_EQ(adapter.get_paired_peripherals().size(), 1u);

    peripheral.disconnect();
    EXPECT_EQ(disconnected_event.wait_for(2s), std::future_status::ready);
    peripheral.set_callback_on_connected(nullptr);
    peripheral.set_callback_on_disconnected(nullptr);
    EXPECT_TRUE(peripheral.is_paired());

    peripheral.unpair();
    EXPECT_FALSE(peripheral.is_paired());
    EXPECT_TRUE(adapter.get_paired_peripherals().empty());
}

TEST_F(Plain, WritesAndDescriptorsStoreBinaryValues) {
    auto peripheral = discover();
    peripheral.connect();

    ByteArray payload{0x00, 0xff, 0x80, 0x01};
    peripheral.write_request(SERVICE, VALUE, payload);
    EXPECT_EQ(peripheral.read(SERVICE, VALUE), payload);

    peripheral.write_command(SERVICE, VALUE, {});
    EXPECT_EQ(peripheral.read(SERVICE, VALUE), ByteArray{});

    peripheral.write(SERVICE, VALUE, DESCRIPTION, payload);
    EXPECT_EQ(peripheral.read(SERVICE, VALUE, DESCRIPTION), payload);

    EXPECT_THROW(peripheral.read("missing", VALUE), Exception::ServiceNotFound);
    EXPECT_THROW(peripheral.read(SERVICE, "missing"), Exception::CharacteristicNotFound);
    EXPECT_THROW(peripheral.read(SERVICE, VALUE, "missing"), Exception::DescriptorNotFound);
    EXPECT_THROW(peripheral.write_request(BATTERY_SERVICE, BATTERY_VALUE, payload), Exception::OperationNotSupported);
    EXPECT_THROW(peripheral.read(SERVICE, ERROR_VALUE), Exception::OperationFailed);
    EXPECT_THROW(peripheral.write_request(SERVICE, ERROR_VALUE, payload), Exception::OperationFailed);

    peripheral.disconnect();
    EXPECT_THROW(peripheral.read(SERVICE, VALUE), Exception::NotConnected);
}

TEST_F(Plain, NotificationsAndIndicationsTrackSubscriptionsAndReconnects) {
    auto peripheral = discover();
    peripheral.connect();

    for (bool indicate : {false, true}) {
        ByteArray payload{0x00, 0xff, static_cast<uint8_t>(indicate)};
        peripheral.write_request(SERVICE, VALUE, payload);

        auto received = std::make_shared<std::promise<ByteArray>>();
        auto result = received->get_future();
        auto once = std::make_shared<std::atomic<bool>>(false);
        auto callback = [received, once](ByteArray value) {
            if (!once->exchange(true)) received->set_value(value);
        };
        if (indicate) {
            peripheral.indicate(SERVICE, VALUE, callback);
        } else {
            peripheral.notify(SERVICE, VALUE, callback);
        }

        EXPECT_EQ(peripheral.read(SERVICE, VALUE, CCCD), (ByteArray{static_cast<uint8_t>(indicate ? 2 : 1), 0}));
        ASSERT_EQ(result.wait_for(2s), std::future_status::ready);
        EXPECT_EQ(result.get(), payload);

        peripheral.unsubscribe(SERVICE, VALUE);
        EXPECT_EQ(peripheral.read(SERVICE, VALUE, CCCD), (ByteArray{0, 0}));
    }

    peripheral.disconnect();
    peripheral.connect();
    EXPECT_EQ(peripheral.read(SERVICE, VALUE, CCCD), (ByteArray{0, 0}));
}

TEST_F(Plain, PowerStateAndCallbacks) {
    adapter.scan_start();
    auto powered_off = std::make_shared<std::promise<void>>();
    auto event = powered_off->get_future();
    adapter.set_callback_on_power_off([powered_off] { powered_off->set_value(); });

    adapter.power_off();
    EXPECT_EQ(event.wait_for(2s), std::future_status::ready);
    EXPECT_FALSE(adapter.is_powered());
    EXPECT_FALSE(Adapter::bluetooth_enabled());
    EXPECT_FALSE(adapter.scan_is_active());
    EXPECT_NO_THROW(adapter.scan_start());
    EXPECT_FALSE(adapter.scan_is_active());

    adapter.power_on();
    EXPECT_TRUE(adapter.is_powered());
    EXPECT_TRUE(Adapter::bluetooth_enabled());
}

TEST_F(Plain, HardwareConfigurationCannotEnableAnotherBackend) {
    const bool previous = Config::Dongl::use_dongl_backend;
    Config::Dongl::use_dongl_backend = true;
    auto backends = Backend::get_backends();
    Config::Dongl::use_dongl_backend = previous;

    ASSERT_EQ(backends.size(), 1u);
    EXPECT_EQ(backends.front().identifier(), "Plain");
}

TEST_F(Plain, ExplicitSimulationUsesTheSameBackend) {
    auto backend = Backend::get_backends().front();
    {
        Simulation::Environment environment;
        environment.add_adapter("Custom", "00:00:00:00:00:01");
        environment.activate();

        ASSERT_EQ(Backend::get_backends().size(), 1u);
        EXPECT_EQ(Backend::get_backends().front().identifier(), "Plain");
        EXPECT_EQ(backend.adapters().front().identifier(), "Custom");
        ASSERT_EQ(Adapter::get_adapters().size(), 1u);
        EXPECT_EQ(Adapter::get_adapters().front().identifier(), "Custom");

        auto custom = Adapter::get_adapters().front();
        custom.power_off();
        EXPECT_FALSE(backend.bluetooth_enabled());
        EXPECT_FALSE(Adapter::bluetooth_enabled());
    }

    EXPECT_EQ(Adapter::get_adapters().front().identifier(), "Plain Adapter");
    EXPECT_EQ(backend.adapters().front().identifier(), "Plain Adapter");
    EXPECT_TRUE(backend.bluetooth_enabled());
}

TEST_F(Plain, LocalGattRoundTripsThroughTheCentralApi) {
    auto local = adapter.create_local_peripheral();
    EXPECT_EQ(local.underlying(), nullptr);

    auto service = local.add_service(SERVICE);
    EXPECT_THROW(local.add_service(SERVICE), Exception::OperationFailed);
    EXPECT_THROW(service.add_characteristic(VALUE, {}), Exception::OperationFailed);
    auto characteristic = service.add_characteristic(
        VALUE, {Local::CharacteristicCapability::READ, Local::CharacteristicCapability::WRITE_REQUEST,
                Local::CharacteristicCapability::WRITE_COMMAND, Local::CharacteristicCapability::NOTIFY,
                Local::CharacteristicCapability::INDICATE});
    local.add_advertised_service(std::vector<BluetoothUUID>{SERVICE});

    ByteArray initial{0x00, 0xff, 0x80};
    characteristic.set_value(initial);
    EXPECT_EQ(characteristic.value(), initial);
    EXPECT_EQ(local.services().front().uuid(), SERVICE);
    EXPECT_EQ(service.characteristics().front().uuid(), VALUE);
    EXPECT_EQ(characteristic.capabilities().size(), 5u);
    EXPECT_THROW(service.add_characteristic(VALUE, {Local::CharacteristicCapability::READ}),
                 Exception::OperationFailed);

    auto connected = std::make_shared<std::promise<BluetoothAddress>>();
    auto connection_event = connected->get_future();
    local.set_callback_on_client_connected([connected](BluetoothAddress address) { connected->set_value(address); });

    local.start();
    EXPECT_TRUE(local.is_started());
    EXPECT_TRUE(local.is_advertising());
    EXPECT_THROW(local.add_service(BATTERY_SERVICE), Exception::OperationFailed);
    EXPECT_THROW(service.add_characteristic(ERROR_VALUE, {}), Exception::OperationFailed);
    EXPECT_THROW(local.remove_all_services(), Exception::OperationFailed);

    auto peer = discover("Plain Adapter Peripheral");
    peer.connect();
    ASSERT_EQ(connection_event.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(connection_event.get(), adapter.address());
    local.set_callback_on_client_connected(nullptr);

    EXPECT_EQ(peer.read(SERVICE, VALUE), initial);
    characteristic.set_callback_on_read([] { return ByteArray{0x12, 0x00, 0xff}; });
    EXPECT_EQ(peer.read(SERVICE, VALUE), (ByteArray{0x12, 0x00, 0xff}));
    characteristic.set_callback_on_read(nullptr);

    characteristic.set_callback_on_write([characteristic](ByteArray value) mutable {
        EXPECT_EQ(characteristic.value(), value);
        characteristic.set_value(ByteArray{0x42});
    });
    peer.write_request(SERVICE, VALUE, initial);
    EXPECT_EQ(peer.read(SERVICE, VALUE), (ByteArray{0x42}));
    peer.write_command(SERVICE, VALUE, initial);
    EXPECT_EQ(peer.read(SERVICE, VALUE), (ByteArray{0x42}));

    characteristic.set_callback_on_write(nullptr);
    peer.write_command(SERVICE, VALUE, initial);
    EXPECT_EQ(peer.read(SERVICE, VALUE), initial);

    for (bool indication : {false, true}) {
        auto subscribed = std::make_shared<std::promise<void>>();
        auto subscribed_event = subscribed->get_future();
        characteristic.set_callback_on_subscribed([subscribed] { subscribed->set_value(); });

        auto received = std::make_shared<std::promise<ByteArray>>();
        auto received_event = received->get_future();
        auto callback = [received](ByteArray value) { received->set_value(value); };
        if (indication) {
            peer.indicate(SERVICE, VALUE, callback);
        } else {
            peer.notify(SERVICE, VALUE, callback);
        }
        ASSERT_EQ(subscribed_event.wait_for(2s), std::future_status::ready);
        characteristic.set_callback_on_subscribed(nullptr);

        characteristic.set_value(initial);
        ASSERT_EQ(received_event.wait_for(2s), std::future_status::ready);
        EXPECT_EQ(received_event.get(), initial);

        auto unsubscribed = std::make_shared<std::promise<void>>();
        auto unsubscribed_event = unsubscribed->get_future();
        characteristic.set_callback_on_unsubscribed([unsubscribed] { unsubscribed->set_value(); });
        peer.unsubscribe(SERVICE, VALUE);
        EXPECT_EQ(unsubscribed_event.wait_for(2s), std::future_status::ready);
        characteristic.set_callback_on_unsubscribed(nullptr);
    }

    auto disconnected = std::make_shared<std::promise<BluetoothAddress>>();
    auto disconnection_event = disconnected->get_future();
    local.set_callback_on_client_disconnected(
        [disconnected](BluetoothAddress address) { disconnected->set_value(address); });
    local.stop();
    EXPECT_FALSE(peer.is_connected());
    EXPECT_FALSE(local.is_advertising());
    EXPECT_FALSE(local.is_started());
    EXPECT_EQ(disconnection_event.wait_for(2s), std::future_status::ready);
    local.set_callback_on_client_disconnected(nullptr);

    EXPECT_THROW(peer.connect(), Exception::OperationFailed);
    local.start();
    peer.connect();
    EXPECT_EQ(peer.read(SERVICE, VALUE), initial);

    peer.disconnect();
    local.stop();
    local.remove_all_services();
    EXPECT_TRUE(local.services().empty());
    EXPECT_THROW(service.add_characteristic(ERROR_VALUE, {}), Exception::InvalidReference);
    EXPECT_THROW(service.characteristics(), Exception::InvalidReference);

    auto replacement = local.add_service(SERVICE).add_characteristic(VALUE, {Local::CharacteristicCapability::READ});
    replacement.set_value(ByteArray{0x99});
    EXPECT_THROW(characteristic.set_value(initial), Exception::InvalidReference);
    EXPECT_EQ(replacement.value(), (ByteArray{0x99}));
}

TEST_F(Plain, LocalSubscriptionsCountClientsAndDisconnects) {
    Simulation::Environment environment;
    environment.add_adapter("Host", "00:00:00:00:00:01");
    environment.add_adapter("Client", "00:00:00:00:00:02");
    environment.activate();

    auto adapters = Adapter::get_adapters();
    auto local = adapters.front().create_local_peripheral();
    auto value = local.add_service(SERVICE).add_characteristic(
        VALUE, {Local::CharacteristicCapability::READ, Local::CharacteristicCapability::NOTIFY});
    auto subscribed = std::make_shared<std::atomic<int>>(0);
    auto unsubscribed = std::make_shared<std::atomic<int>>(0);
    value.set_callback_on_subscribed([subscribed] { ++*subscribed; });
    value.set_callback_on_unsubscribed([unsubscribed] { ++*unsubscribed; });
    local.start();

    std::vector<Peripheral> peers;
    for (auto& scanner : adapters) {
        scanner.scan_for(250);
        auto results = scanner.scan_get_results();
        ASSERT_EQ(results.size(), 1u);
        auto peer = results.front();
        peer.connect();
        peer.notify(SERVICE, VALUE, [](ByteArray) {});
        peers.push_back(peer);
    }

    ASSERT_TRUE(eventually([&] { return subscribed->load() == 1; }));
    peers.front().disconnect();
    EXPECT_EQ(unsubscribed->load(), 0);

    auto received = std::make_shared<std::promise<ByteArray>>();
    auto event = received->get_future();
    peers.back().notify(SERVICE, VALUE, [received](ByteArray bytes) { received->set_value(bytes); });
    value.set_value(ByteArray{0x00, 0xff});
    ASSERT_EQ(event.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(event.get(), (ByteArray{0x00, 0xff}));
    EXPECT_EQ(subscribed->load(), 1);

    local.stop();
    EXPECT_FALSE(peers.back().is_connected());
    EXPECT_TRUE(eventually([&] { return unsubscribed->load() == 1; }));
}

TEST_F(Plain, ReplacingStoppedServicesPreservesPendingUnsubscribeCallbacks) {
    auto local = adapter.create_local_peripheral();
    auto value = local.add_service(SERVICE).add_characteristic(
        VALUE, {Local::CharacteristicCapability::WRITE_COMMAND, Local::CharacteristicCapability::NOTIFY});

    auto subscribed = std::make_shared<std::promise<void>>();
    auto subscribed_event = subscribed->get_future();
    value.set_callback_on_subscribed([subscribed] { subscribed->set_value(); });
    auto unsubscribed = std::make_shared<std::promise<void>>();
    auto unsubscribed_event = unsubscribed->get_future();
    value.set_callback_on_unsubscribed([unsubscribed] { unsubscribed->set_value(); });

    local.start();
    auto peer = discover("Plain Adapter Peripheral");
    peer.connect();
    peer.notify(SERVICE, VALUE, [](ByteArray) {});
    ASSERT_EQ(subscribed_event.wait_for(2s), std::future_status::ready);

    auto entered = std::make_shared<std::promise<void>>();
    auto entered_event = entered->get_future();
    auto release = std::make_shared<std::promise<void>>();
    auto gate = release->get_future().share();
    value.set_callback_on_write([entered, gate](ByteArray) {
        entered->set_value();
        gate.wait_for(2s);
    });
    peer.write_command(SERVICE, VALUE, {});
    ASSERT_EQ(entered_event.wait_for(2s), std::future_status::ready);

    local.stop();
    local.remove_all_services();
    release->set_value();
    EXPECT_EQ(unsubscribed_event.wait_for(2s), std::future_status::ready);
}

TEST_F(Plain, DestroyingLocalHostClosesClientsAndInvalidatesItsChildren) {
    auto local = adapter.create_local_peripheral();
    auto characteristic = local.add_service(SERVICE).add_characteristic(VALUE, {Local::CharacteristicCapability::READ});
    local.start();
    auto peer = discover("Plain Adapter Peripheral");
    peer.connect();

    local = {};
    EXPECT_FALSE(peer.is_connected());
    EXPECT_THROW(characteristic.value(), Exception::InvalidReference);
}

}  // namespace
