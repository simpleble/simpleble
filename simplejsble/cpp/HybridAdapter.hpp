#pragma once

#include "HybridAdapterSpec.hpp"
#include <string>
#include <iostream>

namespace margelo::nitro::simplejsble {

    class HybridAdapter: public HybridAdapterSpec {
        public:
            HybridAdapter(): HybridObject(TAG) {}

            std::string greet(const std::string& name) override;
    };

} // namespace margelo::nitro::simplejsble