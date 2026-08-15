#include <simpleble/SimpleBLE.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>

namespace {

constexpr auto SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0";
constexpr auto CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef1";

std::atomic_bool running{true};

void signal_handler(int) { running = false; }

}  // namespace

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto adapters = SimpleBLE::Adapter::get_adapters();
    if (adapters.empty()) {
        std::cerr << "No Bluetooth adapters found." << std::endl;
        return EXIT_FAILURE;
    }

    auto peripheral = adapters.front().create_local_peripheral();

    SimpleBLE::Local::Advertisement advertisement;
    advertisement.local_name = "SimpleBLE Peripheral";
    peripheral.set_advertisement(advertisement);

    auto service = peripheral.add_service(SERVICE_UUID);
    auto characteristic = service.add_characteristic(
        CHARACTERISTIC_UUID,
        {SimpleBLE::Local::CharacteristicCapability::READ, SimpleBLE::Local::CharacteristicCapability::WRITE_REQUEST,
         SimpleBLE::Local::CharacteristicCapability::WRITE_COMMAND, SimpleBLE::Local::CharacteristicCapability::NOTIFY,
         SimpleBLE::Local::CharacteristicCapability::INDICATE});
    characteristic.set_value(SimpleBLE::ByteArray("ready"));

    characteristic.set_callback_on_write([&characteristic](SimpleBLE::ByteArray value) {
        std::cout << "Write: " << value.toHex() << std::endl;
        // Echo the written value to subscribed clients.
        characteristic.set_value(std::move(value));
    });
    characteristic.set_callback_on_subscribed([]() { std::cout << "Client subscribed." << std::endl; });
    characteristic.set_callback_on_unsubscribed([]() { std::cout << "Client unsubscribed." << std::endl; });

    peripheral.set_callback_on_client_connected(
        [](SimpleBLE::BluetoothAddress address) { std::cout << "Client connected: " << address << std::endl; });
    peripheral.set_callback_on_client_disconnected(
        [](SimpleBLE::BluetoothAddress address) { std::cout << "Client disconnected: " << address << std::endl; });

    peripheral.start();
    std::cout << "Advertising as SimpleBLE Peripheral. Press Ctrl+C to stop." << std::endl;

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    characteristic.set_callback_on_write({});
    peripheral.stop();
    return EXIT_SUCCESS;
}
