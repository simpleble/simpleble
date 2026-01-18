#pragma once

#include "HybridServiceSpec.hpp"
#include <simpleble/SimpleBLE.h>
#include <NitroModules/ArrayBuffer.hpp>
#include <memory>
#include <vector>

namespace margelo::nitro::simplejsble {

class HybridCharacteristicSpec;

class HybridService : public HybridServiceSpec {
  public:
    HybridService() : HybridObject(TAG) {}
    explicit HybridService(SimpleBLE::Service service)
        : HybridObject(TAG), _service(std::move(service)) {}

    bool initialized() override;
    std::string uuid() override;
    std::shared_ptr<ArrayBuffer> data() override;
    std::vector<std::shared_ptr<HybridCharacteristicSpec>> characteristics() override;

    SimpleBLE::Service& getInternal() { return _service; }
    const SimpleBLE::Service& getInternal() const { return _service; }

  private:
    SimpleBLE::Service _service;
};

}  // namespace margelo::nitro::simplejsble
