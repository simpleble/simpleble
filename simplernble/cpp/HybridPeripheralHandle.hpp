#pragma once

#include "Common.hpp"
#include "HybridPeripheralHandleSpec.hpp"

namespace margelo::nitro::simplernble {

class HybridPeripheralHandle : public HybridPeripheralHandleSpec {
  public:
    explicit HybridPeripheralHandle(SimpleBLE::Peripheral peripheral);

    std::string identifier() override;
    std::string address() override;
    AddressType addressType() override;
    std::optional<double> rssi() override;
    std::optional<double> txPower() override;
    double mtu() override;
    bool isConnected() override;
    bool isConnectable() override;
    std::shared_ptr<Promise<bool>> isPaired() override;
    std::vector<ManufacturerDataEntry> manufacturerData() override;
    std::vector<NativeService> services() override;
    std::shared_ptr<Promise<void>> connect() override;
    std::shared_ptr<Promise<void>> disconnect() override;
    std::shared_ptr<Promise<void>> unpair() override;
    std::shared_ptr<Promise<std::shared_ptr<ArrayBuffer>>> read(const std::string& service,
                                                                const std::string& characteristic) override;
    std::shared_ptr<Promise<void>> writeRequest(const std::string& service, const std::string& characteristic,
                                                const std::shared_ptr<ArrayBuffer>& data) override;
    std::shared_ptr<Promise<void>> writeCommand(const std::string& service, const std::string& characteristic,
                                                const std::shared_ptr<ArrayBuffer>& data) override;
    std::shared_ptr<Promise<std::shared_ptr<ArrayBuffer>>> readDescriptor(const std::string& service,
                                                                          const std::string& characteristic,
                                                                          const std::string& descriptor) override;
    std::shared_ptr<Promise<void>> writeDescriptor(const std::string& service, const std::string& characteristic,
                                                   const std::string& descriptor,
                                                   const std::shared_ptr<ArrayBuffer>& data) override;
    std::shared_ptr<Promise<void>> notify(const std::string& service, const std::string& characteristic,
                                          double id) override;
    std::shared_ptr<Promise<void>> indicate(const std::string& service, const std::string& characteristic,
                                            double id) override;
    std::shared_ptr<Promise<void>> unsubscribe(const std::string& service, const std::string& characteristic) override;
    void subscribe(const std::function<void(PeripheralEvent)>& onEvent,
                   const std::function<void(double, const std::shared_ptr<ArrayBuffer>&)>& onValue) override;

  private:
    using ValueListener = std::function<void(double, const std::shared_ptr<ArrayBuffer>&)>;

    SimpleBLE::Peripheral peripheral_;
    std::shared_ptr<Executor::Strand> strand_ = std::make_shared<Executor::Strand>();
    ValueListener on_value_;
};

}  // namespace margelo::nitro::simplernble
