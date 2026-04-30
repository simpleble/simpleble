#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <simplebluez/Bluez.h>
#include <simplebluez/Exceptions.h>
#include "simplebluez/Types.h"

using namespace SimpleDBus;


std::atomic_bool app_running = true;
std::atomic_bool async_thread_active = true;
SimpleBluez::Bluez bluez;


void async_thread_function() {
    while (async_thread_active) {
        bluez.run_async();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void cleanup(
    std::shared_ptr<SimpleBluez::Adapter> adapter,
    std::shared_ptr<SimpleBluez::Advertisement> advertisement,
    std::thread& async_thread
) {
    async_thread_active = false;
    adapter->unregister_advertisement(advertisement);

    if (async_thread.joinable()) async_thread.join();
    std::cout << "Powering off adapter..." << std::endl;
    adapter->powered(false);


    app_running = false;
}

void millisecond_delay(int ms) {
    for (int i = 0; i < ms; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void signal_handler(int signal) { app_running = false; }


int main(int argc, char* argv[]) {
    //std::signal(SIGINT, signal_handler);
    bluez.init();
    std::thread async_thread(async_thread_function);
    auto adapter = bluez.get_adapters()[0];

    if (!adapter->powered()) {
        std::cout << "Powering on adapter..." << std::endl;
        adapter->powered(true);
    }

    std::cout << "Initializing SimpleBluez Peripheral Mode Demo" << std::endl;

    // --- ADAPTER SETUP ---
    adapter->alias("Potato");

    // --- ADVERTISEMENT DEFINITION ---

    auto advertisement = bluez.root_custom()->advertisement_add("fried_potato");
    std::map<uint16_t, SimpleBluez::ByteArray> data;
    data[0x1024] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    advertisement->manufacturer_data(data);
    advertisement->timeout(10);
    advertisement->local_name("SimpleBluez");

    // --- MAIN EVENT LOOP ---
    try {
        while (app_running) {
            // Handle advertising state.
            if (!advertisement->active()) {
                adapter->register_advertisement(advertisement);

                std::cout << "Advertising on " << adapter->identifier()
                          << " [" << adapter->address() << "]" << std::endl;
            }

            // This should eventually become a yield.
            millisecond_delay(100);
        }
    }
    catch (const Exception::SendFailed& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;

        // --- CLEANUP ---
        cleanup( adapter, advertisement, async_thread);

        return 1;
    }

    // --- CLEANUP ---
    cleanup( adapter, advertisement, async_thread);

    return 0;
}
