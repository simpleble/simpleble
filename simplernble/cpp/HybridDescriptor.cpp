#include "HybridDescriptor.hpp"

namespace margelo::nitro::simplernble {

bool HybridDescriptor::initialized() { return _descriptor.initialized(); }

std::string HybridDescriptor::uuid() { return _descriptor.uuid(); }

}  // namespace margelo::nitro::simplernble
