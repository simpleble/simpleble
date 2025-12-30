#include "HybridAdapter.hpp"

namespace margelo::nitro::simplejsble {

std::string HybridAdapter::greet(const std::string& name) { return "Hello, " + name + "!"; }

std::vector<std::shared_ptr<HybridAdapterSpec>> HybridAdapter::get_adapters() {
    std::vector<std::shared_ptr<HybridAdapterSpec>> result;

    for (auto& adapter : SimpleBLE::Adapter::get_adapters()) {
        auto hybrid_adapter = std::make_shared<HybridAdapter>(adapter);
        result.push_back(hybrid_adapter);
    }

    return result;
}

bool HybridAdapter::bluetooth_enabled() { return SimpleBLE::Adapter::bluetooth_enabled(); }

}  // namespace margelo::nitro::simplejsble