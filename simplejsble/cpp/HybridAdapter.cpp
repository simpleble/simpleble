#include "HybridAdapter.hpp"
#include <simpleble/Exceptions.h>
#include <stdexcept>

namespace margelo {
namespace nitro {
namespace simplejsble {

HybridAdapter::HybridAdapter() : HybridAdapterSpec(), adapter_() {}

HybridAdapter::HybridAdapter(SimpleBLE::Adapter adapter) : HybridAdapterSpec(), adapter_(std::move(adapter)) {}

std::vector<std::shared_ptr<HybridAdapterSpec>> HybridAdapter::getAdapters() {
    try {
        auto adapters = SimpleBLE::Adapter::get_adapters();
        std::vector<std::shared_ptr<HybridAdapterSpec>> result;
        result.reserve(adapters.size());
        for (auto& adapter : adapters) {
            result.push_back(convertAdapter(std::move(adapter)));
        }
        return result;
    } catch (const std::exception& e) {
        return {};
    }
}

std::shared_ptr<HybridAdapterSpec> HybridAdapter::convertAdapter(SimpleBLE::Adapter adapter) {
    return std::make_shared<HybridAdapter>(std::move(adapter));
}

}  // namespace simplejsble
}  // namespace nitro
}  // namespace margelo
