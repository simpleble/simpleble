#pragma once

#include "HybridCharacteristicSpec.hpp"
#include <simpleble/SimpleBLE.h>
#include <memory>
#include <vector>
#include <string>

namespace margelo::nitro::simplejsble {

// Forward declaration
class HybridDescriptor;

class HybridCharacteristic : public HybridCharacteristicSpec {
  public:
    HybridCharacteristic() : HybridObject(TAG) {}
    explicit HybridCharacteristic(SimpleBLE::Characteristic characteristic)
        : HybridObject(TAG), _characteristic(std::move(characteristic)) {}

    bool initialized() override;
    std::string uuid() override;
    std::vector<std::shared_ptr<HybridDescriptor>> descriptors() override;
    std::vector<std::string> capabilities() override;
    bool can_read() override;
    bool can_write_request() override;
    bool can_write_command() override;
    bool can_notify() override;
    bool can_indicate() override;

    SimpleBLE::Characteristic& getInternal() { return _characteristic; }
    const SimpleBLE::Characteristic& getInternal() const { return _characteristic; }

  private:
    SimpleBLE::Characteristic _characteristic;
};

}  // namespace margelo::nitro::simplejsble
