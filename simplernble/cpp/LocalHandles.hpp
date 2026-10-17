#pragma once

#include "Common.hpp"
#include "HybridLocalCharacteristicHandleSpec.hpp"
#include "HybridLocalPeripheralHandleSpec.hpp"
#include "HybridLocalServiceHandleSpec.hpp"

namespace margelo::nitro::simplernble {

// NOTE: A local peripheral, its services and its characteristics share one
// strand, so calls on any of them run in call order.
using HostStrand = std::shared_ptr<Executor::Strand>;

class HybridLocalCharacteristicHandle : public HybridLocalCharacteristicHandleSpec {
  public:
    HybridLocalCharacteristicHandle(SimpleBLE::Local::Characteristic characteristic, HostStrand strand);

    std::string uuid() override;
    std::vector<std::string> capabilities() override;
    std::shared_ptr<ArrayBuffer> value() override;
    std::shared_ptr<Promise<void>> setValue(const std::shared_ptr<ArrayBuffer>& value) override;
    std::shared_ptr<Promise<void>> setReadHandler(
        const std::optional<std::function<std::shared_ptr<Promise<std::shared_ptr<Promise<std::vector<double>>>>>()>>&
            handler) override;
    void subscribe(const std::function<void(LocalCharacteristicEvent)>& onEvent,
                   const std::function<void(const std::shared_ptr<ArrayBuffer>&)>& onWrite) override;

  private:
    SimpleBLE::Local::Characteristic characteristic_;
    HostStrand strand_;
};

class HybridLocalServiceHandle : public HybridLocalServiceHandleSpec {
  public:
    HybridLocalServiceHandle(SimpleBLE::Local::Service service, HostStrand strand);

    std::string uuid() override;
    std::shared_ptr<HybridLocalCharacteristicHandleSpec> addCharacteristic(const std::string& uuid,
                                                                           const std::vector<std::string>& capabilities) override;
    std::vector<std::shared_ptr<HybridLocalCharacteristicHandleSpec>> characteristics() override;

  private:
    SimpleBLE::Local::Service service_;
    HostStrand strand_;
};

class HybridLocalPeripheralHandle : public HybridLocalPeripheralHandleSpec {
  public:
    explicit HybridLocalPeripheralHandle(SimpleBLE::Local::Peripheral peripheral);

    void addAdvertisedService(const std::vector<std::string>& uuids) override;
    void setAdvertisedManufacturerData(double companyId, const std::shared_ptr<ArrayBuffer>& data) override;
    void setConnectable(bool connectable) override;
    void setAdvertisementLocalName(const std::optional<std::string>& name) override;
    std::shared_ptr<HybridLocalServiceHandleSpec> addService(const std::string& uuid) override;
    std::vector<std::shared_ptr<HybridLocalServiceHandleSpec>> services() override;
    void removeAllServices() override;
    std::shared_ptr<Promise<void>> start() override;
    std::shared_ptr<Promise<void>> stop() override;
    bool isStarted() override;
    std::shared_ptr<Promise<bool>> isAdvertising() override;
    void subscribe(const std::function<void(LocalPeripheralEvent, const std::string&)>& listener) override;

  private:
    SimpleBLE::Local::Peripheral peripheral_;
    HostStrand strand_ = std::make_shared<Executor::Strand>();
};

}  // namespace margelo::nitro::simplernble
