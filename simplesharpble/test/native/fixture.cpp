// Test-only deterministic backends, routed through the real C/C++ frontends.
#include <simpleble/Logging.h>
#include <simpleble/Peripheral.h>
#include <simpleble/local/Peripheral.h>
#include <simplecble/simplecble.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include "BuilderBase.h"
#include "CharacteristicBase.h"
#include "DescriptorBase.h"
#include "LocalCharacteristicBase.h"
#include "LocalPeripheralBase.h"
#include "LocalServiceBase.h"
#include "PeripheralBase.h"
#include "ServiceBase.h"
#include "external/kvn_safe_callback.hpp"

#ifdef _WIN32
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C" __attribute__((visibility("default")))
#endif
using namespace SimpleBLE;
static const std::string uuid = "0000180f-0000-1000-8000-00805f9b34fb";
static ByteArray payload(size_t length = 257) {
    ByteArray value;
    for (size_t i = 0; i < length; ++i) value.push_back(static_cast<uint8_t>(i));
    return value;
}

struct Remote : PeripheralBase {
    bool connected = false;
    ByteArray data = payload();
    std::mutex mutex;
    std::function<void(ByteArray)> notification;
    std::function<void()> on_connect, on_disconnect;
    void* underlying() const override { return nullptr; }
    std::string identifier() override { return "Fixture \xc3\xa9"; }
    BluetoothAddress address() override { return "01:02:03:04:05:06"; }
    BluetoothAddressType address_type() override { return BluetoothAddressType::RANDOM; }
    int16_t rssi() override { return -70; }
    int16_t tx_power() override { return -32768; }
    uint16_t mtu() override { return 512; }
    void connect() override {
        connected = true;
        if (on_connect) on_connect();
    }
    void disconnect() override {
        connected = false;
        if (on_disconnect) on_disconnect();
    }
    bool is_connected() override { return connected; }
    bool is_connectable() override { return true; }
    bool is_paired() override { return false; }
    void unpair() override {}
    std::vector<std::shared_ptr<ServiceBase>> available_services() override {
        std::vector<std::shared_ptr<DescriptorBase>> descriptors{std::make_shared<DescriptorBase>(uuid)};
        std::vector<std::shared_ptr<CharacteristicBase>> characteristics{
            std::make_shared<CharacteristicBase>(uuid, descriptors, true, true, true, true, true)};
        return {std::make_shared<ServiceBase>(uuid, characteristics)};
    }
    std::vector<std::shared_ptr<ServiceBase>> advertised_services() override {
        return {std::make_shared<ServiceBase>(uuid, data)};
    }
    std::map<uint16_t, ByteArray> manufacturer_data() override { return {{0x1234, data}}; }
    ByteArray read(const BluetoothUUID&, const BluetoothUUID&) override { return data; }
    ByteArray read(const BluetoothUUID&, const BluetoothUUID&, const BluetoothUUID&) override { return data; }
    void write_request(const BluetoothUUID&, const BluetoothUUID&, const ByteArray& value) override { data = value; }
    void write_command(const BluetoothUUID&, const BluetoothUUID&, const ByteArray& value) override { data = value; }
    void write(const BluetoothUUID&, const BluetoothUUID&, const BluetoothUUID&, const ByteArray& value) override {
        data = value;
    }
    void notify(const BluetoothUUID&, const BluetoothUUID&, std::function<void(ByteArray)> value) override {
        std::lock_guard<std::mutex> lock(mutex);
        notification = std::move(value);
    }
    void indicate(const BluetoothUUID& s, const BluetoothUUID& c, std::function<void(ByteArray)> value) override {
        notify(s, c, value);
    }
    void unsubscribe(const BluetoothUUID&, const BluetoothUUID&) override {
        std::lock_guard<std::mutex> lock(mutex);
        notification = nullptr;
    }
    void set_callback_on_connected(std::function<void()> value) override { on_connect = std::move(value); }
    void set_callback_on_disconnected(std::function<void()> value) override { on_disconnect = std::move(value); }
    void emit() {
        std::function<void(ByteArray)> callback;
        {
            std::lock_guard<std::mutex> lock(mutex);
            callback = notification;
        }
        if (callback) callback(data);
    }
};

namespace L = SimpleBLE::Local;
struct LocalCharacteristic : L::CharacteristicBase {
    std::string id;
    std::set<L::CharacteristicCapability> flags;
    ByteArray data;
    kvn::safe_callback<ByteArray()> on_read;
    std::function<void(ByteArray)> on_write;
    std::function<void()> on_subscribe, on_unsubscribe;
    LocalCharacteristic(std::string id, std::set<L::CharacteristicCapability> flags)
        : id(std::move(id)), flags(std::move(flags)) {}
    BluetoothUUID uuid() override { return id; }
    std::set<L::CharacteristicCapability> capabilities() override { return flags; }
    ByteArray value() override { return data; }
    void set_value(ByteArray value) override { data = std::move(value); }
    void set_callback_on_read(std::function<ByteArray()> value) override {
        if (value)
            on_read.load(std::move(value));
        else
            on_read.unload();
    }
    void set_callback_on_write(std::function<void(ByteArray)> value) override { on_write = std::move(value); }
    void set_callback_on_subscribed(std::function<void()> value) override { on_subscribe = std::move(value); }
    void set_callback_on_unsubscribed(std::function<void()> value) override { on_unsubscribe = std::move(value); }
};
struct LocalService : L::ServiceBase {
    std::string id;
    std::vector<std::shared_ptr<L::CharacteristicBase>> values;
    explicit LocalService(std::string id) : id(std::move(id)) {}
    BluetoothUUID uuid() override { return id; }
    std::shared_ptr<L::CharacteristicBase> add_characteristic(BluetoothUUID id,
                                                              std::set<L::CharacteristicCapability> flags) override {
        auto value = std::make_shared<LocalCharacteristic>(id, flags);
        values.push_back(value);
        return value;
    }
    std::vector<std::shared_ptr<L::CharacteristicBase>> characteristics() override { return values; }
};
struct LocalPeripheral : L::PeripheralBase {
    bool started = false;
    std::vector<std::shared_ptr<L::ServiceBase>> values;
    std::function<void(BluetoothAddress)> on_connect, on_disconnect;
    void* underlying() const override { return nullptr; }
    void check() {
        if (started) throw Exception::OperationFailed("Configure before start");
    }
    void add_advertised_service(BluetoothUUID) override { check(); }
    void add_advertised_service(std::vector<BluetoothUUID>) override { check(); }
    std::shared_ptr<L::ServiceBase> add_service(BluetoothUUID id) override {
        check();
        auto value = std::make_shared<LocalService>(id);
        values.push_back(value);
        return value;
    }
    std::vector<std::shared_ptr<L::ServiceBase>> services() override { return values; }
    void remove_all_services() override {
        check();
        values.clear();
    }
    void start() override {
        started = true;
        if (on_connect) on_connect("fixture-client");
    }
    void stop() override {
        started = false;
        if (on_disconnect) on_disconnect("fixture-client");
    }
    bool is_started() override { return started; }
    bool is_advertising() override { return started; }
    void set_callback_on_client_connected(std::function<void(BluetoothAddress)> value) override {
        on_connect = std::move(value);
    }
    void set_callback_on_client_disconnected(std::function<void(BluetoothAddress)> value) override {
        on_disconnect = std::move(value);
    }
};

API void* test_remote() { return new Peripheral(Factory::build(std::make_shared<Remote>())); }
API void* test_clone_remote(void* handle) { return new Peripheral(*static_cast<Peripheral*>(handle)); }
API void test_emit(void* handle) { Factory::get_internal<Remote>(*static_cast<Peripheral*>(handle)).emit(); }
API void* test_local() { return new L::Peripheral(Factory::build(std::make_shared<LocalPeripheral>())); }
API size_t test_read_local(void* handle, uint8_t* target, size_t capacity) {
    auto& value = Factory::get_internal<LocalCharacteristic>(*static_cast<L::Characteristic*>(handle));
    auto bytes = value.on_read ? value.on_read() : value.data;
    if (bytes.size() <= capacity && !bytes.empty()) std::memcpy(target, bytes.data(), bytes.size());
    return bytes.size();
}
API void test_write_local(void* handle, const uint8_t* data, size_t length) {
    auto& value = Factory::get_internal<LocalCharacteristic>(*static_cast<L::Characteristic*>(handle));
    value.data = ByteArray((const char*)data, length);
    if (value.on_write) value.on_write(value.data);
    if (value.on_subscribe) value.on_subscribe();
    if (value.on_unsubscribe) value.on_unsubscribe();
}
API void test_log() {
    Logging::Logger::get()->log(Logging::Level::Error, "fixture", "file", 12, "test", "message \xc3\xa9");
}
API size_t test_layout(int index) {
    const size_t values[] = {sizeof(simpleble_uuid_t),
                             sizeof(simpleble_descriptor_t),
                             sizeof(simpleble_characteristic_t),
                             sizeof(simpleble_service_t),
                             sizeof(simpleble_manufacturer_data_t),
                             offsetof(simpleble_characteristic_t, descriptor_count),
                             offsetof(simpleble_characteristic_t, descriptors),
                             offsetof(simpleble_service_t, data_length),
                             offsetof(simpleble_service_t, data),
                             offsetof(simpleble_service_t, characteristic_count),
                             offsetof(simpleble_service_t, characteristics),
                             offsetof(simpleble_manufacturer_data_t, data_length),
                             offsetof(simpleble_manufacturer_data_t, data)};
    return index >= 0 && index < 13 ? values[index] : 0;
}
