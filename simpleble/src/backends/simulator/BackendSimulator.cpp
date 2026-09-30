#include <string>

#include "AdapterSimulator.h"
#include "BackendUtils.h"
#include "CommonUtils.h"
#include "core/EnvironmentCore.h"

namespace SimpleBLE {

/**
 * Reports the adapters of the active simulation environment, if any.
 */
class BackendSimulator : public BackendSingleton<BackendSimulator> {
  public:
    BackendSimulator(buildToken) {}
    virtual ~BackendSimulator() = default;

    SharedPtrVector<AdapterBase> adapters() override;
    bool bluetooth_enabled() override;
    std::string identifier() const noexcept override;
    bool is_active() override;
};

std::shared_ptr<BackendBase> BACKEND_SIMULATOR() { return BackendSimulator::get(); }

std::string BackendSimulator::identifier() const noexcept { return "Simulator"; }

bool BackendSimulator::bluetooth_enabled() { return true; }

bool BackendSimulator::is_active() { return Simulation::Internal::EnvironmentCore::active() != nullptr; }

SharedPtrVector<AdapterBase> BackendSimulator::adapters() {
    SharedPtrVector<AdapterBase> adapters;
    if (auto environment = Simulation::Internal::EnvironmentCore::active()) {
        for (auto& adapter : environment->adapters()) {
            adapters.push_back(adapter);
        }
    }
    return adapters;
}

}  // namespace SimpleBLE
