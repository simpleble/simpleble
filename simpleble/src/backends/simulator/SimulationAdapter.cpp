#include <simpleble/simulation/Adapter.h>

#include <algorithm>

#include "AdapterSimulator.h"
#include "core/SimulatorUtils.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Simulation;
using namespace SimpleBLE::Simulation::Internal;

Simulation::Adapter::Adapter(std::shared_ptr<AdapterBase> internal) : internal_(std::move(internal)) {}

bool Simulation::Adapter::initialized() const { return internal_ != nullptr; }

std::string Simulation::Adapter::identifier() const { return AdapterSimulator::from(internal_).identifier(); }

BluetoothAddress Simulation::Adapter::address() const { return AdapterSimulator::from(internal_).address(); }

void Simulation::Adapter::set_connection_interval(std::chrono::microseconds interval) {
    auto value = clamp_interval(interval);
    AdapterSimulator::from(internal_).update_policy([&](CentralPolicy& policy) { policy.connection_interval = value; });
}

void Simulation::Adapter::set_accepted_connection_intervals(std::chrono::microseconds min,
                                                            std::chrono::microseconds max) {
    auto range = interval_range(min, max);
    AdapterSimulator::from(internal_).update_policy([&](CentralPolicy& policy) {
        policy.accepted_interval_min = range.first;
        policy.accepted_interval_max = range.second;
    });
}

void Simulation::Adapter::set_max_event_length(std::chrono::microseconds length) {
    auto value = std::max(length, std::chrono::microseconds::zero());
    AdapterSimulator::from(internal_).update_policy([&](CentralPolicy& policy) { policy.max_event_length = value; });
}

void Simulation::Adapter::set_supervision_timeout(std::chrono::milliseconds timeout) {
    auto value = std::max(timeout, std::chrono::milliseconds(100));
    AdapterSimulator::from(internal_).update_policy([&](CentralPolicy& policy) { policy.supervision_timeout = value; });
}

std::shared_ptr<AdapterBase> Simulation::Adapter::internal() const { return internal_; }
