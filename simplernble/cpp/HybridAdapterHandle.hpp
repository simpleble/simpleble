#pragma once

#include "Common.hpp"
#include "HybridAdapterHandleSpec.hpp"

namespace margelo::nitro::simplernble {

class HybridAdapterHandle : public HybridAdapterHandleSpec {
  public:
    explicit HybridAdapterHandle(SimpleBLE::Adapter adapter);

    std::string identifier() override;
    std::string address() override;
    std::shared_ptr<Promise<bool>> isPowered() override;
    std::shared_ptr<Promise<void>> powerOn() override;
    std::shared_ptr<Promise<void>> powerOff() override;
    std::shared_ptr<Promise<void>> scanStart() override;
    std::shared_ptr<Promise<void>> scanStop() override;
    std::shared_ptr<Promise<void>> scanFor(double timeoutMs) override;
    std::shared_ptr<Promise<bool>> scanIsActive() override;
    std::vector<std::shared_ptr<HybridPeripheralHandleSpec>> scanGetResults() override;
    std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>> getPairedPeripherals() override;
    std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>> getConnectedPeripherals() override;
    std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridPeripheralHandleSpec>>>> retrieveCachedPeripherals(
        const std::vector<std::string>& identifiers) override;
    std::shared_ptr<HybridLocalPeripheralHandleSpec> createLocalPeripheral() override;
    void subscribe(const std::function<void(AdapterEvent, const std::optional<std::shared_ptr<HybridPeripheralHandleSpec>>&)>&
                       listener) override;

  private:
    SimpleBLE::Adapter adapter_;
    std::shared_ptr<Executor::Strand> strand_ = std::make_shared<Executor::Strand>();
};

}  // namespace margelo::nitro::simplernble
