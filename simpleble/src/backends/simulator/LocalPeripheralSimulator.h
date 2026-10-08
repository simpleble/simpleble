#pragma once

#include <memory>

#include "LocalPeripheralBase.h"

namespace SimpleBLE {

class AdapterSimulator;

namespace Simulation::Internal {
class EnvironmentCore;
}  // namespace Simulation::Internal

namespace Local {

std::shared_ptr<PeripheralBase> make_simulated_peripheral(
    const std::shared_ptr<Simulation::Internal::EnvironmentCore>& environment,
    const std::shared_ptr<AdapterSimulator>& adapter);

}  // namespace Local
}  // namespace SimpleBLE
