#include <simplebluez/Bluez.h>

#include <poll.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "simplebluez/Types.h"

SimpleBluez::Bluez bluez;

constexpr auto kServiceUuid = "12345678-1234-5678-1234-567812345678";
constexpr auto kCharacteristicUuid = "12345678-aaaa-5678-1234-567812345678";

std::atomic_bool async_thread_active = true;
void async_thread_function() {
    while (async_thread_active) {
        bluez.run_async();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

std::atomic_bool app_running = true;
void signal_handler(int signal) { app_running = false; }

void millisecond_delay(int ms) {
    for (int i = 0; i < ms; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

struct NotifyClient {
    SimpleDBus::UnixSocket socket;
    SimpleBluez::Characteristic::ValueOptions options;
};

void remove_notify_client(std::vector<NotifyClient>& notify_clients, std::vector<NotifyClient>::iterator& client_it,
                          const std::string& reason) {
    if (client_it->options.device.has_value()) {
        std::cout << "Removing notify client " << std::string(*client_it->options.device) << ": " << reason
                  << std::endl;
    } else {
        std::cout << "Removing notify client: " << reason << std::endl;
    }

    client_it = notify_clients.erase(client_it);
}

void send_payload_to_notify_clients(std::vector<NotifyClient>& notify_clients, const SimpleBluez::ByteArray& payload) {
    for (auto client_it = notify_clients.begin(); client_it != notify_clients.end();) {
        auto& client = *client_it;

        if (!client.socket.valid()) {
            remove_notify_client(notify_clients, client_it, "socket is no longer valid");
            continue;
        }

        pollfd socket_poll{};
        socket_poll.fd = client.socket.fd();
        socket_poll.events = POLLOUT | POLLIN | POLLHUP | POLLERR | POLLNVAL;

        int poll_result = poll(&socket_poll, 1, 0);
        if (poll_result < 0) {
            std::cout << "poll() failed: " << std::strerror(errno) << std::endl;
            ++client_it;
            continue;
        }

        if (socket_poll.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            remove_notify_client(notify_clients, client_it, "central stopped notifications or disconnected");
            continue;
        }

        if (socket_poll.revents & POLLIN) {
            std::vector<uint8_t> confirmation(1);
            ssize_t bytes_read = client.socket.receive(confirmation.data(), confirmation.size());
            if (bytes_read == 0) {
                remove_notify_client(notify_clients, client_it, "socket closed while reading indication confirmation");
                continue;
            }
            if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                remove_notify_client(notify_clients, client_it, "failed to read indication confirmation");
                continue;
            }
        }

        if (!(socket_poll.revents & POLLOUT)) {
            ++client_it;
            continue;
        }

        uint16_t mtu = client.options.mtu.value_or(23);
        size_t max_payload = mtu > 3 ? mtu - 3 : 0;
        if (max_payload == 0) {
            remove_notify_client(notify_clients, client_it, "invalid MTU");
            continue;
        }

        SimpleBluez::ByteArray outgoing = payload;
        if (outgoing.size() > max_payload) {
            outgoing = outgoing.slice(0, max_payload);
        }

        ssize_t bytes_sent = client.socket.send(outgoing.data(), outgoing.size());
        if (bytes_sent == static_cast<ssize_t>(outgoing.size())) {
            ++client_it;
            continue;
        }

        if (bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            ++client_it;
            continue;
        }

        if (bytes_sent < 0) {
            remove_notify_client(notify_clients, client_it, std::string("send failed: ") + std::strerror(errno));
            continue;
        }

        remove_notify_client(notify_clients, client_it, "short socket write");
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    bluez.init();
    std::thread async_thread(async_thread_function);
    auto adapter = bluez.get_adapters()[0];

    if (!adapter->powered()) {
        std::cout << "Powering on adapter..." << std::endl;
        adapter->powered(true);
    }

    std::cout << "Initializing SimpleBluez AcquireNotify Peripheral Stream Demo" << std::endl;

    // --- ADAPTER SETUP ---
    std::map<std::string, std::shared_ptr<SimpleBluez::Device>> peripherals;
    std::mutex peripherals_mutex;
    adapter->set_on_device_updated([&peripherals, &peripherals_mutex](std::shared_ptr<SimpleBluez::Device> device) {
        std::scoped_lock lock(peripherals_mutex);

        const bool device_connected = device->connected();
        const bool is_new_device = peripherals.find(device->address()) == peripherals.end();

        if (device_connected && is_new_device) {
            peripherals[device->address()] = device;
            std::cout << "New peripheral: " << device->name() << " [" << device->address() << "]" << std::endl;
        } else if (!device_connected && !is_new_device) {
            peripherals.erase(device->address());
            std::cout << "Lost peripheral: " << device->name() << " [" << device->address() << "]" << std::endl;
        }
    });

    // -- APPLICATION SETUP --
    auto svc_manager = bluez.root_custom()->service_mgr_add("main");

    // --- SERVICE DEFINITION ---
    auto service0 = svc_manager->service_add("my_service");
    service0->uuid(kServiceUuid);
    service0->primary(true);

    auto characteristic0 = service0->characteristic_add("my_characteristic");
    characteristic0->uuid(kCharacteristicUuid);
    characteristic0->flags({"read", "write", "notify"});

    characteristic0->set_on_read_value([characteristic0](SimpleBluez::Characteristic::ValueOptions options) {
        std::cout << "ReadValue called" << std::endl;
        if (options.mtu.has_value()) {
            std::cout << "Read MTU: " << *options.mtu << std::endl;
        }

        characteristic0->value(SimpleBluez::ByteArray("AcquireNotify stream"));
    });

    characteristic0->set_on_write_value(
        [](const SimpleBluez::ByteArray& value, SimpleBluez::Characteristic::ValueOptions options) {
            std::cout << "WriteValue called with value: " << value.toHex() << std::endl;
            if (options.mtu.has_value()) {
                std::cout << "Write MTU: " << *options.mtu << std::endl;
            }
        });

    std::vector<NotifyClient> notify_clients;
    std::mutex notify_clients_mutex;

    characteristic0->set_on_acquire_notify(
        [&notify_clients, &notify_clients_mutex](SimpleDBus::UnixSocket socket,
                                                 SimpleBluez::Characteristic::ValueOptions options) {
            std::scoped_lock lock(notify_clients_mutex);

            if (options.device.has_value()) {
                std::cout << "AcquireNotify from " << std::string(*options.device);
            } else {
                std::cout << "AcquireNotify";
            }

            std::cout << " with MTU " << options.mtu.value_or(23) << std::endl;
            notify_clients.emplace_back(NotifyClient{std::move(socket), options});
        });

    // Export the optional NotifyAcquired property before registration so BlueZ can
    // choose the fd-backed AcquireNotify path for future subscriptions.
    characteristic0->enable_acquire_notify();

    // Register the services and characteristics.
    adapter->register_application(svc_manager->path());

    // NOTE: This long delay is not necessary. However, once an application is registered
    // you want to wait until all services have been added to the adapter. This is done by
    // checking the UUIDs property of org.bluez.Adapter1.
    millisecond_delay(1000);

    // --- ADVERTISEMENT DEFINITION ---

    auto advertisement = bluez.root_custom()->advertisement_add("fried_potato");
    std::map<uint16_t, SimpleBluez::ByteArray> data;
    data[0x1024] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    advertisement->manufacturer_data(data);
    advertisement->service_uuids({kServiceUuid});
    advertisement->timeout(10);
    advertisement->local_name("SimpleBluez");

    auto supported_secondary_channels = adapter->supported_secondary_channels();
    if (std::find(supported_secondary_channels.begin(), supported_secondary_channels.end(), "2M") !=
        supported_secondary_channels.end()) {
        advertisement->secondary_channel("2M");
        std::cout << "Using 2M secondary advertising channel" << std::endl;
    } else {
        std::cout << "2M secondary advertising channel not reported by BlueZ; using default advertising PHY" << std::endl;
    }

    uint32_t sequence = 0;

    // --- MAIN EVENT LOOP ---
    while (app_running) {
        // Handle advertising state.
        if (!advertisement->active()) {
            adapter->register_advertisement(advertisement);
            std::cout << "Advertising on " << adapter->identifier() << " [" << adapter->address() << "]" << std::endl;
        }

        std::string message = "tick:" + std::to_string(sequence++);
        SimpleBluez::ByteArray payload(message);

        {
            std::scoped_lock lock(notify_clients_mutex);
            send_payload_to_notify_clients(notify_clients, payload);
        }

        millisecond_delay(100);
    }

    // --- CLEANUP ---

    characteristic0->clear_on_acquire_notify();
    adapter->clear_on_device_updated();

    {
        std::scoped_lock lock(notify_clients_mutex);
        notify_clients.clear();
    }

    std::vector<std::shared_ptr<SimpleBluez::Device>> peripherals_to_disconnect;
    {
        std::scoped_lock lock(peripherals_mutex);
        for (const auto& peripheral : peripherals) {
            peripherals_to_disconnect.push_back(peripheral.second);
        }
        peripherals.clear();
    }

    for (auto& peripheral : peripherals_to_disconnect) {
        std::cout << "Disconnecting from " << peripheral->name() << " [" << peripheral->address() << "]" << std::endl;
        peripheral->disconnect();
    }

    adapter->unregister_advertisement(advertisement);
    adapter->unregister_application(svc_manager->path());

    async_thread_active = false;
    async_thread.join();

    return 0;
}
