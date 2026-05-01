#include <gtest/gtest.h>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "backends/common/PeripheralBase.h"
#include "backends/common/ServiceBase.h"
#include "simpleble/Peripheral.h"
#include "simplecble/peripheral.h"
#include "simplecble/types.h"

using namespace SimpleBLE;

class MockPeripheralBase : public PeripheralBase {
  public:
    MockPeripheralBase() = default;
    virtual ~MockPeripheralBase() = default;

    void* underlying() const override { return nullptr; }
    std::string identifier() override { return ""; }
    BluetoothAddress address() override { return ""; }
    BluetoothAddressType address_type() override { return BluetoothAddressType::PUBLIC; }
    int16_t rssi() override { return 0; }
    int16_t tx_power() override { return 0; }
    uint16_t mtu() override { return 0; }
    void connect() override {}
    void disconnect() override {}
    bool is_connected() override { return false; }
    bool is_connectable() override { return false; }
    bool is_paired() override { return false; }
    void unpair() override {}

    std::vector<std::shared_ptr<ServiceBase>> available_services() override { return {}; }
    std::vector<std::shared_ptr<ServiceBase>> advertised_services() override { return {}; }

    std::map<uint16_t, ByteArray> manufacturer_data() override {
        // Return > 27 bytes to test buffer overflow
        ByteArray large_data(std::string(50, 'A'));
        return {{0x1234, large_data}};
    }

    ByteArray read(BluetoothUUID const&, BluetoothUUID const&) override { return ""; }
    void write_request(BluetoothUUID const&, BluetoothUUID const&, ByteArray const&) override {}
    void write_command(BluetoothUUID const&, BluetoothUUID const&, ByteArray const&) override {}
    void notify(BluetoothUUID const&, BluetoothUUID const&, std::function<void(ByteArray)>) override {}
    void indicate(BluetoothUUID const&, BluetoothUUID const&, std::function<void(ByteArray)>) override {}
    void unsubscribe(BluetoothUUID const&, BluetoothUUID const&) override {}
    ByteArray read(BluetoothUUID const&, BluetoothUUID const&, BluetoothUUID const&) override { return ""; }
    void write(BluetoothUUID const&, BluetoothUUID const&, BluetoothUUID const&, ByteArray const&) override {}
    void set_callback_on_connected(std::function<void()>) override {}
    void set_callback_on_disconnected(std::function<void()>) override {}
};

class MockPeripheralBaseService : public MockPeripheralBase {
  public:
    std::vector<std::shared_ptr<ServiceBase>> advertised_services() override {
        return {std::make_shared<ServiceBase>("1234", ByteArray(std::string(50, 'B')))};
    }
};

class MockPeripheral : public SimpleBLE::Peripheral {
  public:
    MockPeripheral(std::shared_ptr<SimpleBLE::PeripheralBase> base) { this->internal_ = base; }
};

TEST(BufferOverflowTest, ManufacturerDataOverflow) {
    auto base = std::make_shared<MockPeripheralBase>();
    MockPeripheral mp(base);
    simpleble_peripheral_t handle = static_cast<simpleble_peripheral_t>(&mp);

    simpleble_manufacturer_data_t out_data;
    simpleble_err_t err = simpleble_peripheral_manufacturer_data_get(handle, 0, &out_data);
    EXPECT_EQ(err, SIMPLEBLE_SUCCESS);
    // Even though size is 50, memory should not be corrupted.
    // And length should report actual size 50 so user knows it was truncated in the 27 byte array.
    EXPECT_EQ(out_data.data_length, 50);
}

TEST(BufferOverflowTest, ServicesOverflow) {
    auto base = std::make_shared<MockPeripheralBaseService>();
    MockPeripheral mp(base);
    simpleble_peripheral_t handle = static_cast<simpleble_peripheral_t>(&mp);

    simpleble_service_t out_data;
    simpleble_err_t err = simpleble_peripheral_services_get(handle, 0, &out_data);
    EXPECT_EQ(err, SIMPLEBLE_SUCCESS);
    EXPECT_EQ(out_data.data_length, 50);
}
