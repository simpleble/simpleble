#include <simpleble/simulation/Environment.h>

#include <simpleble/Exceptions.h>

#include "AdapterSimulator.h"
#include "core/DeviceCore.h"
#include "core/EnvironmentCore.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Simulation;
using namespace SimpleBLE::Simulation::Internal;

Environment::Environment() : internal_(std::make_shared<EnvironmentCore>()) {}

Environment::~Environment() { internal_->shutdown(); }

Simulation::Adapter Environment::add_adapter(const std::string& identifier, const BluetoothAddress& address) {
    return Simulation::Adapter(internal_->add_adapter(identifier, address));
}

void Environment::add_device(std::shared_ptr<Device> device) { internal_->add_device(std::move(device)); }

Link Environment::link(const Simulation::Adapter& adapter, const std::shared_ptr<Device>& device) {
    if (!device) throw Exception::NotInitialized();
    return Link(internal_->link(&AdapterSimulator::from(adapter.internal()), device->internal().get()));
}

void Environment::activate() { internal_->activate(); }

void Environment::deactivate() { internal_->deactivate(); }

bool Environment::is_active() const { return internal_->is_active(); }
