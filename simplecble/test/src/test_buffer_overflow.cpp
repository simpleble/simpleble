#include <gtest/gtest.h>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "backends/common/CharacteristicBase.h"
#include "backends/common/DescriptorBase.h"
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
    BluetoothUUID uuid = "1234";

    std::vector<std::shared_ptr<ServiceBase>> advertised_services() override {
        return {std::make_shared<ServiceBase>(uuid, ByteArray(std::string(50, 'B')))};
    }

    bool is_connected() override { return connected; }
    bool connected = false;

    std::vector<std::shared_ptr<ServiceBase>> available_services() override {
        std::vector<std::shared_ptr<DescriptorBase>> descriptors = {std::make_shared<DescriptorBase>(uuid)};
        std::vector<std::shared_ptr<CharacteristicBase>> characteristics = {
            std::make_shared<CharacteristicBase>(uuid, descriptors, true, false, false, false, false)};
        return {std::make_shared<ServiceBase>(uuid, characteristics)};
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

TEST(BufferOverflowTest, ServiceCharacteristicAndDescriptorUUIDs) {
    auto base = std::make_shared<MockPeripheralBaseService>();
    base->connected = true;
    MockPeripheral mp(base);
    simpleble_peripheral_t handle = static_cast<simpleble_peripheral_t>(&mp);

    for (const std::string uuid : {"", "1234", "12345678", "12345678-1234-5678-1234-567812345678"}) {
        SCOPED_TRACE(uuid);
        base->uuid = uuid;
        simpleble_service_t service;
        memset(&service, 0xFF, sizeof(service));

        ASSERT_EQ(simpleble_peripheral_services_get(handle, 0, &service), SIMPLEBLE_SUCCESS);
        EXPECT_STREQ(service.uuid.value, uuid.c_str());
        ASSERT_EQ(service.characteristic_count, 1);
        EXPECT_STREQ(service.characteristics[0].uuid.value, uuid.c_str());
        ASSERT_EQ(service.characteristics[0].descriptor_count, 1);
        EXPECT_STREQ(service.characteristics[0].descriptors[0].uuid.value, uuid.c_str());
    }
}
