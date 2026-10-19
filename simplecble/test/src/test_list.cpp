#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "simpleble/Simulation.h"
#include "simplecble/simplecble.h"

namespace sim = SimpleBLE::Simulation;
using namespace std::chrono_literals;

namespace {

class Beacon : public sim::Device {
  public:
    Beacon(const std::string& name, const std::string& address) : sim::Device(name, address) {
        set_advertising_interval(20ms);
    }
};

std::string name_of(simpleble_peripheral_t peripheral) {
    simpleble_error_t* error = nullptr;
    char* name = simpleble_peripheral_identifier(peripheral, &error);
    std::string result = name ? name : "";
    simpleble_free(name);
    simpleble_error_release(&error);
    return result;
}

std::vector<std::string> names_in(simpleble_peripheral_list_t list) {
    std::vector<std::string> names;
    for (size_t i = 0; i < simpleble_peripheral_list_count(list); i++) {
        simpleble_error_t* error = nullptr;
        simpleble_peripheral_t peripheral = simpleble_peripheral_list_get(list, i, &error);
        EXPECT_EQ(error, nullptr);
        names.push_back(name_of(peripheral));
        simpleble_peripheral_release_handle(peripheral);
        simpleble_error_release(&error);
    }
    return names;
}

size_t scan_results_count(simpleble_adapter_t adapter) {
    simpleble_error_t* error = nullptr;
    simpleble_peripheral_list_t list = nullptr;
    size_t count = simpleble_adapter_scan_get_results(adapter, &list, &error);
    simpleble_peripheral_list_release(list);
    simpleble_error_release(&error);
    return count;
}

bool wait_for_scan_results(simpleble_adapter_t adapter, size_t count) {
    auto deadline = std::chrono::steady_clock::now() + 3s;
    while (scan_results_count(adapter) != count) {
        if (std::chrono::steady_clock::now() > deadline) return false;
        std::this_thread::sleep_for(5ms);
    }
    return true;
}

class ListTest : public ::testing::Test {
  protected:
    void SetUp() override {
        host = environment.add_adapter("sim0", "00:11:22:33:44:55");
        environment.add_device<Beacon>("B", "20:00:00:00:00:02");
        environment.add_device<Beacon>("C", "30:00:00:00:00:03");
        early = environment.add_device<Beacon>("A", "10:00:00:00:00:01");
        environment.link(host, early).set_in_range(false);
        environment.activate();

        simpleble_error_t* error = nullptr;
        simpleble_adapter_list_t adapters = nullptr;
        size_t count = simpleble_adapter_get_adapters(&adapters, &error);
        ASSERT_EQ(error, nullptr);
        for (size_t i = 0; i < count; i++) {
            simpleble_adapter_t candidate = simpleble_adapter_list_get(adapters, i, &error);
            ASSERT_EQ(error, nullptr);
            char* identifier = simpleble_adapter_identifier(candidate, &error);
            if (adapter == nullptr && identifier != nullptr && std::string(identifier) == "sim0") {
                adapter = candidate;
            } else {
                simpleble_adapter_release_handle(candidate);
            }
            simpleble_free(identifier);
        }
        simpleble_adapter_list_release(adapters);
        simpleble_error_release(&error);
        ASSERT_NE(adapter, nullptr);
    }

    void TearDown() override {
        simpleble_error_t* error = nullptr;
        if (adapter != nullptr) simpleble_adapter_scan_stop(adapter, &error);
        simpleble_adapter_release_handle(adapter);
        simpleble_error_release(&error);
    }

    sim::Environment environment;
    sim::Adapter host;
    std::shared_ptr<Beacon> early;
    simpleble_adapter_t adapter = nullptr;
};

}  // namespace

// A device whose address sorts first used to shift every later index between the count and get calls.
TEST_F(ListTest, ScanResultsStayFixedWhileTheScanFindsMore) {
    simpleble_error_t* error = nullptr;
    simpleble_adapter_scan_start(adapter, &error);
    ASSERT_EQ(error, nullptr);
    ASSERT_TRUE(wait_for_scan_results(adapter, 2));

    simpleble_peripheral_list_t list = nullptr;
    EXPECT_EQ(simpleble_adapter_scan_get_results(adapter, &list, &error), 2);
    ASSERT_EQ(error, nullptr);

    environment.link(host, early).set_in_range(true);
    ASSERT_TRUE(wait_for_scan_results(adapter, 3));
    EXPECT_EQ(names_in(list), (std::vector<std::string>{"B", "C"}));

    simpleble_peripheral_list_t later = nullptr;
    simpleble_adapter_scan_get_results(adapter, &later, &error);
    EXPECT_EQ(names_in(later), (std::vector<std::string>{"A", "B", "C"}));
    simpleble_peripheral_list_release(later);
    simpleble_peripheral_list_release(list);
    simpleble_error_release(&error);
}
