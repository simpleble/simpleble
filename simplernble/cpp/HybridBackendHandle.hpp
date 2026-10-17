#pragma once

#include "Common.hpp"
#include "HybridBackendHandleSpec.hpp"

namespace margelo::nitro::simplernble {

class HybridBackendHandle : public HybridBackendHandleSpec {
  public:
    explicit HybridBackendHandle(SimpleBLE::Backend backend);

    std::string identifier() override;
    std::shared_ptr<Promise<bool>> bluetoothEnabled() override;
    std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridAdapterHandleSpec>>>> adapters() override;

  private:
    SimpleBLE::Backend backend_;
};

}  // namespace margelo::nitro::simplernble
