#include <cstdlib>
#include <iostream>
#include <optional>

#include "simpleble/SimpleBLE.h"

int main() {
    std::optional<SimpleBLE::Adapter> selected_adapter;
    for (auto adapter : SimpleBLE::Adapter::get_adapters()) {
        if (adapter.supports_peripheral_server()) {
            selected_adapter = adapter;
            break;
        }
    }

    if (!selected_adapter.has_value()) {
        std::cerr << "No adapter with peripheral-server support found." << std::endl;
        return EXIT_FAILURE;
    }

    auto adapter = selected_adapter.value();
    auto server = adapter.create_peripheral_server();

    const SimpleBLE::BluetoothUUID service_uuid = "12345678-1234-5678-1234-567812345678";
    const SimpleBLE::BluetoothUUID characteristic_uuid = "12345678-AAAA-5678-1234-567812345678";
    SimpleBLE::ByteArray value = {0x00};

    SimpleBLE::CharacteristicDefinition characteristic;
    characteristic.uuid = characteristic_uuid;
    characteristic.properties = {
        SimpleBLE::CharacteristicProperty::READ,
        SimpleBLE::CharacteristicProperty::WRITE_REQUEST,
        SimpleBLE::CharacteristicProperty::NOTIFY,
    };
    characteristic.value = value;
    characteristic.on_read = [&](SimpleBLE::ReadRequest) {
        std::cout << "Read request received." << std::endl;
        return SimpleBLE::ReadResponse::success(value);
    };
    characteristic.on_write = [&](SimpleBLE::WriteRequest request) {
        value = request.value();
        std::cout << "Write request received: " << value << std::endl;
        return SimpleBLE::WriteResponse::success();
    };
    characteristic.on_subscribed = [](SimpleBLE::Central central) {
        std::cout << "Central subscribed: " << central.identifier() << std::endl;
    };
    characteristic.on_unsubscribed = [](SimpleBLE::Central central) {
        std::cout << "Central unsubscribed: " << central.identifier() << std::endl;
    };

    SimpleBLE::ServiceDefinition service;
    service.uuid = service_uuid;
    service.characteristics = {characteristic};
    server.add_service(service);

    server.set_callback_on_central_connected([](SimpleBLE::Central central) {
        std::cout << "Central connected: " << central.identifier() << std::endl;
    });
    server.set_callback_on_central_disconnected([](SimpleBLE::Central central) {
        std::cout << "Central disconnected: " << central.identifier() << std::endl;
    });

    SimpleBLE::AdvertisingData advertising_data;
    advertising_data.local_name = "SimpleBLE";
    advertising_data.service_uuids = {service_uuid};
    server.start_advertising(advertising_data);

    std::cout << "Peripheral server advertising as \"" << advertising_data.local_name << "\"." << std::endl;
    std::cout << "Press Enter to notify subscribers and stop." << std::endl;
    std::cin.get();

    server.notify(service_uuid, characteristic_uuid, value);
    server.stop();

    return EXIT_SUCCESS;
}
