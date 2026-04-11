#include <iostream>

#include "simpleble/Adapter.h"
#include "simpleble/Backend.h"
#include "simpleble/Utils.h"

int main() {
    std::cout << "Using SimpleBLE version: " << SimpleBLE::get_simpleble_version() << std::endl;

    auto backends = SimpleBLE::Backend::get_backends();

    if (backends.empty()) {
        std::cout << "No backends found" << std::endl;
        return EXIT_FAILURE;
    }

    for (auto& backend : backends) {
        std::cout << "Backend: " << backend.identifier() << std::endl;
        
        auto adapter_list = backend.adapters();

        if (adapter_list.empty()) {
            std::cout << "    No adapters found" << std::endl;
            continue;
        }

        for (auto& adapter : adapter_list) {
            std::cout << "    Adapter: " << adapter.identifier() << " [" << adapter.address() << "]" << std::endl;
        }
    }

    std::cout << "---" << std::endl;
    std::cout << "Consolidated Adapter List (SimpleBLE::Adapter::get_adapters()): " << std::endl;
    auto all_adapters = SimpleBLE::Adapter::get_adapters();
    if (all_adapters.empty()) {
        std::cout << "    No adapters found" << std::endl;
    } else {
        for (auto& adapter : all_adapters) {
            std::cout << "    Adapter: " << adapter.identifier() << " [" << adapter.address() << "]" << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
