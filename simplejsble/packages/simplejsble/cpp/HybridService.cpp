#include "HybridService.hpp"
#include "HybridCharacteristic.hpp"

namespace margelo::nitro::simplejsble {

bool HybridService::initialized() {
    return _service.initialized();
}

std::string HybridService::uuid() {
    return _service.uuid();
}

std::shared_ptr<ArrayBuffer> HybridService::data() {
    SimpleBLE::ByteArray bytes = _service.data();
    return ArrayBuffer::copy(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
}

std::vector<std::shared_ptr<HybridCharacteristicSpec>> HybridService::characteristics() {
    std::vector<SimpleBLE::Characteristic> service_characteristics = _service.characteristics();
    std::vector<std::shared_ptr<HybridCharacteristicSpec>> hybrid_characteristics;
    hybrid_characteristics.reserve(service_characteristics.size());
    
    for (auto& characteristic : service_characteristics) {
        hybrid_characteristics.push_back(std::make_shared<HybridCharacteristic>(std::move(characteristic)));
    }
    
    return hybrid_characteristics;
}

}  // namespace margelo::nitro::simplejsble
