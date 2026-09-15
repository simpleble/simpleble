#include <gtest/gtest.h>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "backends/common/CharacteristicBase.h"
#include "backends/common/DescriptorBase.h"
#include "backends/common/PeripheralBase.h"
#include "backends/common/ServiceBase.h"
#include "simpleble/Peripheral.h"
#include "simplecble/error.h"
#include "simplecble/peripheral.h"
#include "simplecble/types.h"

using namespace SimpleBLE;

class MockPeripheralBase : public PeripheralBase {
  public:
    MockPeripheralBase() = default;
    ByteArray payload = std::string(257, 'A');
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

    std::map<uint16_t, ByteArray> manufacturer_data() override { return {{0x1234, payload}}; }

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
        return {std::make_shared<ServiceBase>(uuid, payload)};
    }

    bool is_connected() override { return connected; }
    bool connected = false;
    size_t characteristic_count = 1;
    size_t descriptor_count = 1;
    bool invalid_descriptor = false;

    std::vector<std::shared_ptr<ServiceBase>> available_services() override {
        std::vector<std::shared_ptr<DescriptorBase>> descriptors(descriptor_count,
                                                                 std::make_shared<DescriptorBase>(uuid));
        if (invalid_descriptor) descriptors.push_back(nullptr);
        std::vector<std::shared_ptr<CharacteristicBase>> characteristics(
            characteristic_count,
            std::make_shared<CharacteristicBase>(uuid, descriptors, true, false, false, false, false));
        return {std::make_shared<ServiceBase>(uuid, characteristics)};
    }
};

class MockPeripheral : public SimpleBLE::Peripheral {
  public:
    MockPeripheral(std::shared_ptr<SimpleBLE::PeripheralBase> base) { this->internal_ = base; }
};

TEST(PeripheralDataTest, CompletePayloadsAndEmptyResults) {
    auto base = std::make_shared<MockPeripheralBaseService>();
    MockPeripheral peripheral(base);
    simpleble_error_t* error = nullptr;

    for (size_t length : {0, 27, 28, 257}) {
        SCOPED_TRACE(length);
        std::string expected(length, '\0');
        for (size_t i = 0; i < length; i++) expected[i] = static_cast<char>(i);
        base->payload = expected;

        simpleble_manufacturer_data_t manufacturer = {};
        simpleble_peripheral_manufacturer_data_get(&peripheral, 0, &manufacturer, &error);
        ASSERT_EQ(error, nullptr);
        EXPECT_EQ(manufacturer.manufacturer_id, 0x1234);
        ASSERT_EQ(manufacturer.data_length, length);

        simpleble_service_t service = {};
        simpleble_peripheral_services_get(&peripheral, 0, &service, &error);
        ASSERT_EQ(error, nullptr);
        EXPECT_STREQ(service.uuid.value, "1234");
        ASSERT_EQ(service.data_length, length);
        EXPECT_EQ(service.characteristic_count, 0);
        EXPECT_EQ(service.characteristics, nullptr);

        // Returned buffers are owned copies, independent of later advertisements.
        base->payload.clear();
        if (length == 0) {
            EXPECT_EQ(manufacturer.data, nullptr);
            EXPECT_EQ(service.data, nullptr);
        } else {
            EXPECT_EQ(std::memcmp(manufacturer.data, expected.data(), length), 0);
            EXPECT_EQ(std::memcmp(service.data, expected.data(), length), 0);
        }

        simpleble_service_release(&service);
        EXPECT_EQ(service.data, nullptr);
        EXPECT_EQ(service.data_length, 0);
        EXPECT_EQ(service.uuid.value[0], '\0');
        simpleble_manufacturer_data_release(&manufacturer);
        EXPECT_EQ(manufacturer.data, nullptr);
        EXPECT_EQ(manufacturer.data_length, 0);
        EXPECT_EQ(manufacturer.manufacturer_id, 0);
        simpleble_service_release(&service);
        simpleble_manufacturer_data_release(&manufacturer);
    }
    simpleble_error_release(&error);
}

TEST(PeripheralDataTest, CompleteGattCollectionsAndUUIDs) {
    auto base = std::make_shared<MockPeripheralBaseService>();
    base->connected = true;
    base->characteristic_count = 19;
    base->descriptor_count = 21;
    MockPeripheral peripheral(base);
    simpleble_error_t* error = nullptr;
    simpleble_service_t service = {};

    for (const std::string uuid : {"", "1234", "12345678", "12345678-1234-5678-1234-567812345678"}) {
        SCOPED_TRACE(uuid);
        base->uuid = uuid;
        simpleble_peripheral_services_get(&peripheral, 0, &service, &error);
        ASSERT_EQ(error, nullptr);
        EXPECT_STREQ(service.uuid.value, uuid.c_str());
        EXPECT_EQ(service.data_length, 0);
        EXPECT_EQ(service.data, nullptr);
        ASSERT_EQ(service.characteristic_count, 19);
        for (size_t i = 0; i < service.characteristic_count; i++) {
            const auto& characteristic = service.characteristics[i];
            EXPECT_STREQ(characteristic.uuid.value, uuid.c_str());
            EXPECT_TRUE(characteristic.can_read);
            EXPECT_FALSE(characteristic.can_write_request);
            EXPECT_FALSE(characteristic.can_write_command);
            EXPECT_FALSE(characteristic.can_notify);
            EXPECT_FALSE(characteristic.can_indicate);
            ASSERT_EQ(characteristic.descriptor_count, 21);
            for (size_t j = 0; j < characteristic.descriptor_count; j++) {
                EXPECT_STREQ(characteristic.descriptors[j].uuid.value, uuid.c_str());
            }
        }
        simpleble_service_release(&service);
        EXPECT_EQ(service.characteristic_count, 0);
        EXPECT_EQ(service.characteristics, nullptr);
    }

    base->descriptor_count = 0;
    simpleble_peripheral_services_get(&peripheral, 0, &service, &error);
    ASSERT_EQ(error, nullptr);
    for (size_t i = 0; i < service.characteristic_count; i++) {
        EXPECT_EQ(service.characteristics[i].descriptor_count, 0);
        EXPECT_EQ(service.characteristics[i].descriptors, nullptr);
    }
    simpleble_service_release(&service);
    simpleble_error_release(&error);
}

TEST(PeripheralDataTest, FailedConstructionReleasesPartialService) {
    auto base = std::make_shared<MockPeripheralBaseService>();
    base->connected = true;
    base->characteristic_count = 19;
    base->invalid_descriptor = true;
    MockPeripheral peripheral(base);
    simpleble_error_t* error = nullptr;
    simpleble_service_t service = {};

    simpleble_peripheral_services_get(&peripheral, 0, &service, &error);
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(simpleble_error_code(error), SIMPLEBLE_ERROR_OBJECT_NOT_INITIALIZED);
    EXPECT_EQ(service.uuid.value[0], '\0');
    EXPECT_EQ(service.data_length, 0);
    EXPECT_EQ(service.data, nullptr);
    EXPECT_EQ(service.characteristic_count, 0);
    EXPECT_EQ(service.characteristics, nullptr);
    simpleble_service_release(&service);
    simpleble_error_release(&error);
}
