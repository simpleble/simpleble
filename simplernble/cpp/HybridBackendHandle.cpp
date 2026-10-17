#include "HybridBackendHandle.hpp"

#include "HybridAdapterHandle.hpp"

namespace margelo::nitro::simplernble {

HybridBackendHandle::HybridBackendHandle(SimpleBLE::Backend backend) : HybridObject(TAG), backend_(std::move(backend)) {}

std::string HybridBackendHandle::identifier() {
    return guard([&] { return backend_.identifier(); });
}

std::shared_ptr<Promise<bool>> HybridBackendHandle::bluetoothEnabled() {
    auto backend = backend_;
    return run<bool>(lifecycle(), [backend]() mutable { return backend.bluetooth_enabled(); });
}

std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridAdapterHandleSpec>>>> HybridBackendHandle::adapters() {
    auto backend = backend_;
    return run<std::vector<std::shared_ptr<HybridAdapterHandleSpec>>>(lifecycle(), [backend]() mutable {
        std::vector<std::shared_ptr<HybridAdapterHandleSpec>> handles;
        for (auto& adapter : backend.adapters()) handles.push_back(intern<HybridAdapterHandle>(adapter));
        return handles;
    });
}

}  // namespace margelo::nitro::simplernble
