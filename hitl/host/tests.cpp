#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cctype>
#include <deque>
#include <functional>
#include <limits>
#include <iostream>
#include <mutex>
#include <thread>
#include "fixture.h"

using namespace std::chrono_literals;
using hitl::Bytes;
using hitl::uuid;
static hitl::Options options;
static const std::string battery_service = "0000180f-0000-1000-8000-00805f9b34fb";
static const std::string battery_value = "00002a19-0000-1000-8000-00805f9b34fb";
static const std::string description = "00002901-0000-1000-8000-00805f9b34fb";

struct Packets {
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<Bytes> packets;
    void push(Bytes data) {
        std::lock_guard<std::mutex> lock(mutex);
        packets.push_back(std::move(data));
        changed.notify_one();
    }
    Bytes take() {
        std::unique_lock<std::mutex> lock(mutex);
        if (!changed.wait_for(lock, 5s, [&] { return !packets.empty(); }))
            throw std::runtime_error("Missing BLE callback");
        auto data = packets.front();
        packets.pop_front();
        return data;
    }
    bool empty() {
        std::lock_guard<std::mutex> lock(mutex);
        return packets.empty();
    }
};
Bytes pattern(unsigned length) {
    Bytes data;
    for (unsigned i = 0; i < length; ++i) data.push_back(static_cast<uint8_t>((i * 79) & 255));
    return data;
}
Bytes token_payload(unsigned token) {
    Bytes data{1};
    for (unsigned i = 0; i < 4; ++i) data.push_back(static_cast<uint8_t>((token >> (8 * i)) & 255));
    return data;
}
Bytes packet(unsigned test, unsigned stream, unsigned sequence, unsigned length) {
    Bytes data;
    for (auto n : {test, stream, sequence})
        for (unsigned i = 0; i < 4; ++i) data.push_back(static_cast<uint8_t>((n >> (8 * i)) & 255));
    for (unsigned i = 12; i < length; ++i) data.push_back(static_cast<uint8_t>((sequence + i) & 255));
    return data;
}

class Hardware : public testing::TestWithParam<unsigned> {
  protected:
    std::unique_ptr<hitl::Fixture> fixture;
    unsigned test_id;
    std::string dropped;
    void SetUp() override {
        static std::atomic<unsigned> next{1};
        test_id = next++;
        fixture = std::make_unique<hitl::Fixture>(options);
        if (fixture->uses_rtt()) dropped = fixture->control("STATUS").at("dropped");
        fixture->reset(test_id, GetParam());
        auto identity = fixture->control("HELLO");
        RecordProperty("board", identity.at("board"));
        RecordProperty("firmware", identity.at("build"));
        RecordProperty("control", options.control);
        RecordProperty("simpleble", SimpleBLE::get_simpleble_version());
        auto status = fixture->control("STATUS");
        if (dropped.empty()) dropped = status.at("dropped");
        RecordProperty("att_mtu", status.at("mtu"));
        EXPECT_LE(std::stoul(status.at("mtu")), GetParam());
        auto info = fixture->peer.read(uuid(1), uuid(2));
        ASSERT_EQ(info.size(), 16u);
        auto board = std::stoull(fixture->board, nullptr, 16);
        for (unsigned i = 0; i < 8; ++i) ASSERT_EQ(info[4 + i], static_cast<uint8_t>(board >> (8 * i)));
        EXPECT_EQ(status.at("test"), std::to_string(test_id));
        EXPECT_EQ(fixture->peer.mtu(), std::stoul(status.at("mtu")) - 3);
    }
    void TearDown() override {
        if (fixture && fixture->peer.initialized() && fixture->peer.is_connected()) {
            if (fixture->uses_rtt()) EXPECT_EQ(fixture->control("STATUS").at("dropped"), dropped);
            EXPECT_NO_THROW(fixture->disconnect());
        }
        fixture.reset();
    }
    SimpleBLE::Peripheral& peer() { return fixture->peer; }
    hitl::Fields control(const std::string& command) { return fixture->control(command); }
    void recovered_delivery() {
        auto queue = std::make_shared<Packets>();
        peer().notify(uuid(1), uuid(6), [queue](Bytes data) { queue->push(std::move(data)); });
        hitl::wait_until([&] { return control("STATUS").at("cccd_a") == "1"; });
        control("SEND NOTIFY_A 00ff01");
        EXPECT_EQ(queue->take(), (Bytes{0, 255, 1}));
        delivered("NOTIFY_A", 1);
        peer().unsubscribe(uuid(1), uuid(6));
    }
    // A profile applies when the fixture next advertises, so select it and disconnect.
    void advertise(const std::string& profile, unsigned token, unsigned duration_ms = 0) {
        auto command = "ADV " + profile + " " + std::to_string(token);
        if (duration_ms) command += " " + std::to_string(duration_ms);
        control(command);
        fixture->disconnect();
    }
    void scan_until(const std::function<bool()>& predicate, std::chrono::seconds timeout = 15s) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!predicate()) {
            if (std::chrono::steady_clock::now() >= deadline) throw std::runtime_error("Advertisement not observed");
            fixture->adapter.scan_for(1000);
        }
    }
    bool advertises_token(unsigned token) {
        auto data = peer().manufacturer_data();
        return data.count(0xffff) && data.at(0xffff) == token_payload(token);
    }
    std::string fixture_name() {
        auto suffix = fixture->board.substr(8);
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) { return std::toupper(c); });
        return "SBH-" + suffix;
    }
    void delivered(const std::string& attribute, unsigned count) {
        hitl::Fields state;
        hitl::wait_until([&] {
            state = control("WORK " + attribute);
            return state.at("active") == "0";
        });
        EXPECT_EQ(state.at("sent"), std::to_string(count));
        EXPECT_EQ(state.at("completed"), std::to_string(count));
        EXPECT_EQ(state.at("reason"), "done");
    }
};

TEST_P(Hardware, Discovery) {
    auto info = peer().read(uuid(1), uuid(2));
    ASSERT_EQ(info.size(), 16u);
    Bytes expected{1, 0, 0, 0};
    auto board = std::stoull(fixture->board, nullptr, 16);
    for (unsigned i = 0; i < 8; ++i) expected.push_back(static_cast<uint8_t>((board >> (8 * i)) & 255));
    for (unsigned i = 0; i < 4; ++i) expected.push_back(static_cast<uint8_t>((test_id >> (8 * i)) & 255));
    EXPECT_EQ(info, expected);
    std::map<std::string, SimpleBLE::Service> services;
    for (auto service : peer().services()) services.emplace(service.uuid(), service);
    ASSERT_TRUE(services.count(uuid(1)));
    EXPECT_TRUE(services.count(uuid(0x11)));
    EXPECT_TRUE(services.count(battery_service));
    auto chars = services.at(uuid(1)).characteristics();
    ASSERT_EQ(chars.size(), 8u);
    for (auto characteristic : chars) {
        unsigned suffix = std::stoul(characteristic.uuid().substr(4, 4), nullptr, 16);
        ASSERT_GE(suffix, 2u);
        ASSERT_LE(suffix, 9u);
        EXPECT_EQ(characteristic.can_read(), suffix != 5);
        EXPECT_EQ(characteristic.can_write_request(), suffix == 4 || suffix == 9);
        EXPECT_EQ(characteristic.can_write_command(), suffix == 5);
        EXPECT_EQ(characteristic.can_notify(), suffix == 6 || suffix == 7);
        EXPECT_EQ(characteristic.can_indicate(), suffix == 8);
        if (suffix == 3 || (suffix >= 6 && suffix <= 8)) {
            std::vector<std::string> descriptors;
            for (auto descriptor : characteristic.descriptors()) descriptors.push_back(descriptor.uuid());
            if (suffix == 3) {
                EXPECT_NE(std::find(descriptors.begin(), descriptors.end(), description), descriptors.end());
                EXPECT_NE(std::find(descriptors.begin(), descriptors.end(), uuid(0x10)), descriptors.end());
            } else
                EXPECT_NE(std::find(descriptors.begin(), descriptors.end(), "00002902-0000-1000-8000-00805f9b34fb"),
                          descriptors.end());
        }
    }
}
TEST_P(Hardware, Reads) {
    for (auto length : {0u, 1u, 20u, 21u, 244u, 512u}) {
        SCOPED_TRACE(length);
        auto data = pattern(length);
        control("SET READ_VALUE " + hitl::hex(data));
        EXPECT_EQ(peer().read(uuid(1), uuid(3)), data);
        EXPECT_EQ(hitl::unhex(control("GET READ_VALUE").at("data")), data);
    }
}
TEST_P(Hardware, Writes) {
    for (unsigned suffix : {4u, 5u}) {
        auto attribute = suffix == 4 ? "WRITE_REQUEST" : "WRITE_COMMAND";
        for (auto length : {unsigned(peer().mtu()), 1u, 3u}) {
            auto data = pattern(length);
            auto count = std::stoul(control(std::string("GET ") + attribute).at("writes"));
            if (suffix == 4)
                peer().write_request(uuid(1), uuid(suffix), data);
            else
                peer().write_command(uuid(1), uuid(suffix), data);
            hitl::Fields received;
            hitl::wait_until([&] {
                received = control(std::string("GET_WRITE ") + attribute);
                return std::stoul(received.at("writes")) > count;
            });
            EXPECT_EQ(std::stoul(received.at("writes")), count + 1);
            EXPECT_EQ(hitl::unhex(received.at("data")), data);
            EXPECT_EQ(received.at("op"), suffix == 4 ? "1" : "2");
            if (suffix == 4) EXPECT_EQ(peer().read(uuid(1), uuid(4)), data);
            control(std::string("SET ") + attribute + " ff");
            EXPECT_EQ(hitl::unhex(control(std::string("GET_WRITE ") + attribute).at("data")), data);
        }
    }
}
TEST_P(Hardware, Descriptors) {
    EXPECT_EQ(peer().read(uuid(1), uuid(3), description), Bytes("SimpleBLE HITL read"));
    auto value = Bytes{0, 255, 68};
    peer().write(uuid(1), uuid(3), uuid(0x10), value);
    EXPECT_EQ(peer().read(uuid(1), uuid(3), uuid(0x10)), value);
    EXPECT_EQ(hitl::unhex(control("GET DESC").at("data")), value);
}
TEST_P(Hardware, Errors) {
    EXPECT_THROW(peer().read(uuid(1), uuid(9)), SimpleBLE::Exception::BaseException);
    EXPECT_THROW(peer().write_request(uuid(1), uuid(9), Bytes{1}), SimpleBLE::Exception::BaseException);
    control("SET WRITE_REQUEST 00ff");
    EXPECT_THROW(peer().write_request(uuid(1), uuid(4), pattern(peer().mtu() + 1)),
                 SimpleBLE::Exception::BaseException);
    EXPECT_EQ(peer().read(uuid(1), uuid(4)), (Bytes{0, 255}));
    for (const auto* command :
         {"SET INFO 00", "SET BATTERY 65", "SET READ_VALUE zz", "SEND NOTIFY_A 00", "MTU 99", "UNKNOWN"})
        EXPECT_THROW(control(command), std::runtime_error) << command;
    control("SET READ_VALUE 00ff01");
    EXPECT_EQ(peer().read(uuid(1), uuid(3)), (Bytes{0, 255, 1}));
}
TEST_P(Hardware, NotificationsAndIndications) {
    std::vector<std::shared_ptr<Packets>> queues;
    for (unsigned suffix : {6u, 7u, 8u}) {
        auto queue = std::make_shared<Packets>();
        queues.push_back(queue);
        auto callback = [queue](Bytes data) { queue->push(std::move(data)); };
        if (suffix == 8)
            peer().indicate(uuid(1), uuid(suffix), callback);
        else
            peer().notify(uuid(1), uuid(suffix), callback);
    }
    hitl::wait_until([&] {
        auto state = control("STATUS");
        return state.at("cccd_a") == "1" && state.at("cccd_b") == "1" && state.at("cccd_i") == "2";
    });
    auto length = unsigned(peer().mtu());
    const char* attributes[] = {"NOTIFY_A", "NOTIFY_B", "INDICATE"};
    for (unsigned i = 0; i < 3; ++i)
        control(std::string("STREAM ") + attributes[i] + " " + std::to_string(i + 1) + " " + std::to_string(length) +
                " 4 20");
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned sequence = 0; sequence < 4; ++sequence)
            EXPECT_EQ(queues[i]->take(), packet(test_id, i + 1, sequence, length));
        delivered(attributes[i], 4);
        peer().unsubscribe(uuid(1), uuid(i + 6));
    }
    std::this_thread::sleep_for(150ms);
    for (auto queue : queues) EXPECT_TRUE(queue->empty());
    peer().notify(uuid(1), uuid(6), [queue = queues[0]](Bytes data) { queue->push(std::move(data)); });
    control("SEND NOTIFY_A 00ff01");
    EXPECT_EQ(queues[0]->take(), (Bytes{0, 255, 1}));
    delivered("NOTIFY_A", 1);
    peer().unsubscribe(uuid(1), uuid(6));
    std::this_thread::sleep_for(150ms);
    EXPECT_TRUE(queues[0]->empty());
}
TEST_P(Hardware, Battery) {
    EXPECT_EQ(peer().read(battery_service, battery_value), (Bytes{50}));
    auto queue = std::make_shared<Packets>();
    peer().notify(battery_service, battery_value, [queue](Bytes data) { queue->push(std::move(data)); });
    hitl::wait_until([&] { return control("STATUS").at("cccd_battery") == "1"; });
    control("SEND BATTERY 33");
    EXPECT_EQ(queue->take(), (Bytes{51}));
    delivered("BATTERY", 1);
    peer().unsubscribe(battery_service, battery_value);
    EXPECT_EQ(peer().read(battery_service, battery_value), (Bytes{51}));
}
TEST_P(Hardware, Reconnect) {
    auto connected = std::make_shared<std::atomic<bool>>(false);
    auto disconnected = std::make_shared<std::atomic<bool>>(false);
    peer().set_callback_on_connected([connected] { *connected = true; });
    peer().set_callback_on_disconnected([disconnected] { *disconnected = true; });
    fixture->disconnect();
    hitl::wait_until([&] { return disconnected->load(); });
    fixture->connect();
    hitl::wait_until([&] { return connected->load(); });
    EXPECT_TRUE(peer().is_connected());
    fixture->disconnect();
    fixture->discover();
    fixture->connect();
    EXPECT_EQ(peer().read(uuid(1), uuid(2)).size(), 16u);
}
TEST_P(Hardware, DisconnectDuringStream) {
    auto queue = std::make_shared<Packets>();
    auto disconnected = std::make_shared<std::atomic<bool>>(false);
    peer().set_callback_on_disconnected([disconnected] { *disconnected = true; });
    peer().notify(uuid(1), uuid(6), [queue](Bytes data) { queue->push(std::move(data)); });
    hitl::wait_until([&] { return control("STATUS").at("cccd_a") == "1"; });
    control("STREAM NOTIFY_A 99 20 1000 20");
    EXPECT_EQ(queue->take(), packet(test_id, 99, 0, 20));
    control("DISCONNECT 500");
    hitl::wait_until([&] { return disconnected->load(); }, 10s);
    EXPECT_FALSE(peer().is_connected());
    fixture->connect();
    EXPECT_EQ(control("STATUS").at("active"), "0");
    control("SET READ_VALUE ff00");
    EXPECT_EQ(peer().read(uuid(1), uuid(3)), (Bytes{255, 0}));
    recovered_delivery();
}
TEST_P(Hardware, RebootDuringStream) {
    auto before = control("HELLO").at("boot");
    auto queue = std::make_shared<Packets>();
    auto disconnected = std::make_shared<std::atomic<bool>>(false);
    peer().set_callback_on_disconnected([disconnected] { *disconnected = true; });
    peer().notify(uuid(1), uuid(6), [queue](Bytes data) { queue->push(std::move(data)); });
    hitl::wait_until([&] { return control("STATUS").at("cccd_a") == "1"; });
    control("STREAM NOTIFY_A 99 20 1000 20");
    EXPECT_EQ(queue->take(), packet(test_id, 99, 0, 20));
    control("REBOOT 500");
    hitl::wait_until([&] { return disconnected->load(); }, 10s);
    fixture->discover();
    fixture->connect();
    EXPECT_NE(control("HELLO").at("boot"), before);
    EXPECT_EQ(control("STATUS").at("active"), "0");
    dropped = control("STATUS").at("dropped");
    Bytes initial;
    for (unsigned i = 0; i < 32; ++i) initial.push_back(static_cast<uint8_t>(i));
    EXPECT_EQ(peer().read(uuid(1), uuid(3)), initial);
    recovered_delivery();
}
// Advertising does not depend on the MTU, so these run once.
class Advertising : public Hardware {};

TEST_P(Advertising, Default) {
    const unsigned token = 0x5a000000 + test_id;
    advertise("default", token);
    scan_until([&] { return advertises_token(token); });
    EXPECT_EQ(peer().identifier(), fixture_name());
    EXPECT_TRUE(peer().is_connectable());
    EXPECT_EQ(peer().tx_power(), 0);
    auto services = peer().services();
    EXPECT_TRUE(std::any_of(services.begin(), services.end(), [](auto service) { return service.uuid() == uuid(1); }));
}
TEST_P(Advertising, ServiceData) {
    const unsigned token = 0x5b000000 + test_id;
    advertise("service_data", token);
    scan_until([&] {
        auto services = peer().services();
        return std::any_of(services.begin(), services.end(), [&](auto service) {
            return service.uuid() == uuid(1) && service.data() == token_payload(token);
        });
    });
    // This profile has no manufacturer data; the previous advertisement's entry must not linger.
    EXPECT_EQ(peer().manufacturer_data().count(0xffff), 0u);
    EXPECT_TRUE(peer().is_connectable());
}
TEST_P(Advertising, WithoutTxPower) {
    const unsigned token = 0x5c000000 + test_id;
    advertise("no_tx_power", token);
    scan_until([&] { return advertises_token(token); });
    // Scan again so the new scan response, which omits TX power, has been received.
    fixture->adapter.scan_for(2000);
    EXPECT_EQ(peer().identifier(), fixture_name());
    EXPECT_EQ(peer().tx_power(), std::numeric_limits<int16_t>::min());
}
TEST_P(Advertising, NonscannableDropsScanResponse) {
    // TX power is only in the scan response, so it shows whether a stale scan response is still being reported.
    const unsigned token = 0x5e000000 + test_id;
    advertise("default", token);
    scan_until([&] { return advertises_token(token) && peer().tx_power() == 0; });
    fixture->connect();
    // The fixture restores the default profile after 20 s.
    advertise("nonscannable", token + 1, 20000);
    scan_until([&] { return advertises_token(token + 1) && !peer().is_connectable(); });
    EXPECT_EQ(peer().tx_power(), std::numeric_limits<int16_t>::min());
    scan_until([&] { return peer().is_connectable(); }, 30s);
}
TEST_P(Advertising, NonconnectableTimesOut) {
    const unsigned token = 0x5d000000 + test_id;
    // The fixture restores the connectable profile after 20 s.
    advertise("nonconnectable", token, 20000);
    scan_until([&] { return advertises_token(token) && !peer().is_connectable(); });
    auto start = std::chrono::steady_clock::now();
    EXPECT_THROW(peer().connect(), SimpleBLE::Exception::BaseException);
    // One bounded attempt, not a retry loop that outlasts the profile.
    EXPECT_LT(std::chrono::steady_clock::now() - start, 15s);
    EXPECT_FALSE(peer().is_connected());
    scan_until([&] { return peer().is_connectable(); }, 30s);
    fixture->connect();
    EXPECT_EQ(control("STATUS").at("connected"), "1");
}
INSTANTIATE_TEST_SUITE_P(Mtu, Hardware, testing::Values(23, 247));
INSTANTIATE_TEST_SUITE_P(Mtu, Advertising, testing::Values(247));

TEST(ControlEncoding, BinaryRoundTrip) {
    EXPECT_EQ(hitl::unhex(hitl::hex(pattern(512))), pattern(512));
    EXPECT_EQ(hitl::hex({}), "-");
    EXPECT_TRUE(hitl::unhex("-").empty());
    EXPECT_THROW(hitl::unhex("xyz"), std::runtime_error);
    EXPECT_THROW(hitl::unhex("f"), std::runtime_error);
}
TEST(ControlEncoding, FragmentedRequestAndPagedReply) {
    const auto request = "1 SET READ_VALUE " + hitl::hex(pattern(512)) + "\n";
    const auto reply = "1 OK data=" + hitl::hex(pattern(512)) + "\n";
    std::string received;
    unsigned offset = 0;
    auto write = [&](const Bytes& data) {
        ASSERT_LE(data.size(), 20u);
        ASSERT_GE(data.size(), 2u);
        if (data[0] == 0) {
            for (size_t i = 1; i < data.size(); ++i) received += static_cast<char>(data[i]);
        } else {
            ASSERT_EQ(data.size(), 3u);
            ASSERT_EQ(data[0], 1u);
            offset = data[1] | (unsigned(data[2]) << 8);
        }
    };
    auto read = [&] {
        Bytes page{static_cast<uint8_t>(reply.size()), static_cast<uint8_t>(reply.size() >> 8),
                   static_cast<uint8_t>(offset), static_cast<uint8_t>(offset >> 8)};
        for (size_t i = offset; i < std::min(size_t(offset + 16), reply.size()); ++i)
            page.push_back(static_cast<uint8_t>(reply[i]));
        return page;
    };
    EXPECT_EQ(hitl::exchange_ble(request, write, read), reply);
    EXPECT_EQ(received, request);
}
TEST(ControlEncoding, RejectsMalformedPages) {
    for (const auto& page : {Bytes{}, Bytes{1, 0, 0}, Bytes{0, 0, 0, 0}, Bytes{1, 0, 1, 0, 65}, Bytes{1, 0, 0, 0},
                             Bytes{1, 6, 0, 0, 65}}) {
        EXPECT_THROW(hitl::exchange_ble("1 HELLO\n", [](const Bytes&) {}, [&] { return page; }), std::runtime_error);
    }
    EXPECT_THROW(
        hitl::exchange_ble(std::string(1537, 'x'), [](const Bytes&) {}, [] { return Bytes{}; }), std::runtime_error);
}
TEST(ControlEncoding, RejectsReplyLengthChange) {
    unsigned reads = 0;
    EXPECT_THROW(hitl::exchange_ble(
                     "1 HELLO\n", [](const Bytes&) {},
                     [&] {
                         Bytes page = reads++ == 0 ? Bytes{17, 0, 0, 0} : Bytes{18, 0, 16, 0};
                         for (unsigned i = 0; i < (reads == 1 ? 16u : 2u); ++i) page.push_back(uint8_t{'x'});
                         return page;
                     }),
                 std::runtime_error);
}
TEST(ControlEncoding, StreamPayloadLayout) {
    EXPECT_EQ(packet(1, 2, 3, 20), (Bytes{1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 15, 16, 17, 18, 19, 20, 21, 22}));
    EXPECT_EQ(pattern(512).size(), 512u);
}
int main(int argc, char** argv) {
    try {
        options = hitl::Options::parse(argc, argv);
        testing::InitGoogleTest(&argc, argv);
        if (argc != 1)
            throw std::runtime_error(
                "Unknown HITL option; use --control=ble|rtt --board=<id> --probe=<serial> --adapter=<index>");
        return RUN_ALL_TESTS();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
