#pragma once

#include "HybridDescriptorSpec.hpp"
#include <simpleble/SimpleBLE.h>
#include <string>

namespace margelo::nitro::simplejsble {

class HybridDescriptor : public HybridDescriptorSpec {
  public:
    HybridDescriptor() : HybridObject(TAG) {}
    explicit HybridDescriptor(SimpleBLE::Descriptor descriptor)
        : HybridObject(TAG), _descriptor(std::move(descriptor)) {}

    bool initialized() override;
    std::string uuid() override;

    SimpleBLE::Descriptor& getInternal() { return _descriptor; }
    const SimpleBLE::Descriptor& getInternal() const { return _descriptor; }

  private:
    SimpleBLE::Descriptor _descriptor;
};

}  // namespace margelo::nitro::simplejsble
