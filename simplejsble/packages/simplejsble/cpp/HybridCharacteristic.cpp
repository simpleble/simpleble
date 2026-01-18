#include "HybridCharacteristic.hpp"
#include "HybridDescriptor.hpp"

namespace margelo::nitro::simplejsble {

bool HybridCharacteristic::initialized() {
    return _characteristic.initialized();
}

std::string HybridCharacteristic::uuid() {
    return _characteristic.uuid();
}

std::vector<std::shared_ptr<HybridDescriptorSpec>> HybridCharacteristic::descriptors() {
    std::vector<SimpleBLE::Descriptor> characteristic_descriptors = _characteristic.descriptors();
    std::vector<std::shared_ptr<HybridDescriptorSpec>> hybrid_descriptors;
    hybrid_descriptors.reserve(characteristic_descriptors.size());
    
    for (auto& descriptor : characteristic_descriptors) {
        hybrid_descriptors.push_back(std::make_shared<HybridDescriptor>(std::move(descriptor)));
    }
    
    return hybrid_descriptors;
}

std::vector<std::string> HybridCharacteristic::capabilities() {
    return _characteristic.capabilities();
}

bool HybridCharacteristic::can_read() {
    return _characteristic.can_read();
}

bool HybridCharacteristic::can_write_request() {
    return _characteristic.can_write_request();
}

bool HybridCharacteristic::can_write_command() {
    return _characteristic.can_write_command();
}

bool HybridCharacteristic::can_notify() {
    return _characteristic.can_notify();
}

bool HybridCharacteristic::can_indicate() {
    return _characteristic.can_indicate();
}

}  // namespace margelo::nitro::simplejsble
