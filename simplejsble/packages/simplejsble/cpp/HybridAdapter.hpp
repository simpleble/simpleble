#pragma once

#include "HybridAdapterSpec.hpp"
#include <simpleble/SimpleBLE.h>
#include <string>

namespace margelo::nitro::simplejsble {

class HybridAdapter : public HybridAdapterSpec {
  public:
    HybridAdapter() : HybridObject(TAG) {}
    HybridAdapter(SimpleBLE::Adapter adapter) : HybridObject(TAG), _adapter(adapter) {}

    std::string greet(const std::string& name) override;
    std::vector<std::shared_ptr<HybridAdapterSpec>> get_adapters() override;
    bool bluetooth_enabled() override;

  private:
    SimpleBLE::Adapter _adapter;
};

}  // namespace margelo::nitro::simplejsble