#include <gtest/gtest.h>

#include <simpleble/SimpleBLE.h>

using namespace SimpleBLE;

TEST(PeripheralServerTest, CentralAndRequestTypesCarryContext) {
    Central central("central-1", "AA:BB:CC:DD:EE:FF", BluetoothAddressType::PUBLIC, 185);

    EXPECT_TRUE(central.initialized());
    EXPECT_EQ(central.identifier(), "central-1");
    EXPECT_EQ(central.address(), "AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(central.address_type(), BluetoothAddressType::PUBLIC);
    EXPECT_EQ(central.mtu(), 185);

    ReadRequest read_request(central, "service", "characteristic", std::nullopt, 3, 185);
    EXPECT_EQ(read_request.central().identifier(), "central-1");
    EXPECT_EQ(read_request.service(), "service");
    EXPECT_EQ(read_request.characteristic(), "characteristic");
    EXPECT_FALSE(read_request.descriptor().has_value());
    EXPECT_EQ(read_request.offset(), 3);
    EXPECT_EQ(read_request.mtu(), 185);

    ByteArray payload = {0x01, 0x02};
    WriteRequest write_request(central, "service", "characteristic", BluetoothUUID("descriptor"), payload, 4, true,
                               false, 185);
    EXPECT_EQ(write_request.value().toHex(), payload.toHex());
    ASSERT_TRUE(write_request.descriptor().has_value());
    EXPECT_EQ(*write_request.descriptor(), "descriptor");
    EXPECT_TRUE(write_request.response_required());
    EXPECT_FALSE(write_request.prepared());
}

TEST(PeripheralServerTest, ResponsesRepresentSuccessAndErrors) {
    ByteArray payload = {0x01, 0x02};
    auto read_success = ReadResponse::success(payload);

    EXPECT_TRUE(read_success.is_success());
    EXPECT_EQ(read_success.value().toHex(), payload.toHex());
    EXPECT_EQ(read_success.error(), AttributeError::SUCCESS);

    auto read_error = ReadResponse::error(AttributeError::INVALID_OFFSET);
    EXPECT_FALSE(read_error.is_success());
    EXPECT_EQ(read_error.error(), AttributeError::INVALID_OFFSET);

    auto write_success = WriteResponse::success();
    EXPECT_TRUE(write_success.is_success());
    EXPECT_EQ(write_success.error(), AttributeError::SUCCESS);

    auto write_error = WriteResponse::error(AttributeError::WRITE_NOT_PERMITTED);
    EXPECT_FALSE(write_error.is_success());
    EXPECT_EQ(write_error.error(), AttributeError::WRITE_NOT_PERMITTED);
}

TEST(PeripheralServerTest, PlainBackendSupportsInMemoryLifecycle) {
    auto adapters = Adapter::get_adapters();
    ASSERT_FALSE(adapters.empty());
    ASSERT_TRUE(adapters.at(0).supports_peripheral_server());

    auto server = adapters.at(0).create_peripheral_server();
    EXPECT_TRUE(server.initialized());
    EXPECT_FALSE(server.is_active());
    EXPECT_FALSE(server.is_advertising());

    int started_count = 0;
    int advertising_started_count = 0;
    int central_connected_count = 0;
    int central_disconnected_count = 0;
    int stopped_count = 0;

    server.set_callback_on_started([&]() { started_count++; });
    server.set_callback_on_advertising_started([&]() { advertising_started_count++; });
    server.set_callback_on_central_connected([&](Central central) {
        EXPECT_TRUE(central.initialized());
        central_connected_count++;
    });
    server.set_callback_on_central_disconnected([&](Central central) {
        EXPECT_TRUE(central.initialized());
        central_disconnected_count++;
    });
    server.set_callback_on_stopped([&]() { stopped_count++; });

    const BluetoothUUID service_uuid = "12345678-1234-5678-1234-567812345678";
    const BluetoothUUID characteristic_uuid = "12345678-AAAA-5678-1234-567812345678";

    CharacteristicDefinition characteristic;
    characteristic.uuid = characteristic_uuid;
    characteristic.properties = {
        CharacteristicProperty::READ,
        CharacteristicProperty::WRITE_REQUEST,
        CharacteristicProperty::NOTIFY,
    };

    ServiceDefinition service;
    service.uuid = service_uuid;
    service.characteristics = {characteristic};
    server.add_service(service);

    auto services = server.services();
    ASSERT_EQ(services.size(), 1);
    EXPECT_EQ(services.at(0).uuid, service_uuid);
    ASSERT_EQ(services.at(0).characteristics.size(), 1);
    EXPECT_EQ(services.at(0).characteristics.at(0).uuid, characteristic_uuid);

    AdvertisingData advertising_data;
    advertising_data.local_name = "SimpleBLE";
    advertising_data.service_uuids = {service_uuid};
    server.start_advertising(advertising_data);

    EXPECT_TRUE(server.is_active());
    EXPECT_TRUE(server.is_advertising());
    EXPECT_EQ(started_count, 1);
    EXPECT_EQ(advertising_started_count, 1);
    EXPECT_EQ(central_connected_count, 1);
    EXPECT_EQ(server.centrals().size(), 1);

    EXPECT_NO_THROW(server.notify(service_uuid, characteristic_uuid, ByteArray({0x01})));
    EXPECT_NO_THROW(server.indicate(server.centrals().at(0), service_uuid, characteristic_uuid, ByteArray({0x02})));

    server.stop();

    EXPECT_FALSE(server.is_active());
    EXPECT_FALSE(server.is_advertising());
    EXPECT_EQ(central_disconnected_count, 1);
    EXPECT_EQ(stopped_count, 1);
}
