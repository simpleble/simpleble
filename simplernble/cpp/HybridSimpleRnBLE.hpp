#pragma once

#include "Common.hpp"
#include "HybridSimpleRnBLESpec.hpp"

namespace margelo::nitro::simplernble {

// The module's entry point, which JavaScript creates by name.
class HybridSimpleRnBLE : public HybridSimpleRnBLESpec {
  public:
    HybridSimpleRnBLE() : HybridObject(TAG) {}
    ~HybridSimpleRnBLE() override;

    std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridBackendHandleSpec>>>> getBackends() override;
    std::shared_ptr<Promise<std::shared_ptr<HybridBackendHandleSpec>>> getNativeBackend() override;
    std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridAdapterHandleSpec>>>> getAdapters() override;
    std::shared_ptr<Promise<bool>> bluetoothEnabled() override;

    bool getNegotiateMtu() override;
    void setNegotiateMtu(bool enabled) override;
    ConnectionPriorityRequest getConnectionPriorityRequest() override;
    void setConnectionPriorityRequest(ConnectionPriorityRequest request) override;
    void resetConfig() override;

    double getLogLevel() override;
    void setLogLevel(double level) override;
    void setLogCallback(const std::optional<std::function<void(const LogEntry&)>>& callback) override;
    void logToSystem() override;
    void logToFile(const std::optional<std::string>& path) override;
};

}  // namespace margelo::nitro::simplernble
