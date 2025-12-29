#pragma once

#include "HybridAdapterSpec.hpp"
#include <simpleble/Adapter.h>
#include <memory>
#include <vector>

namespace margelo {
namespace nitro {
namespace simplejsble {

  /**
   * HybridAdapter wraps SimpleBLE::Adapter for use in Nitro.
   * 
   * Minimal POC implementation with only getAdapters() method.
   */
  class HybridAdapter : public HybridAdapterSpec {
  public:
    /**
     * Default constructor for autolinking.
     * Creates an uninitialized HybridAdapter.
     */
    HybridAdapter();

    /**
     * Construct a HybridAdapter wrapping a SimpleBLE::Adapter.
     * 
     * @param adapter The SimpleBLE::Adapter to wrap
     */
    explicit HybridAdapter(SimpleBLE::Adapter adapter);

    // Implement HybridAdapterSpec interface
    std::vector<std::shared_ptr<HybridAdapterSpec>> getAdapters() override;

  private:
    SimpleBLE::Adapter adapter_;
    
    // Helper to convert SimpleBLE::Adapter to HybridAdapterSpec
    static std::shared_ptr<HybridAdapterSpec> convertAdapter(SimpleBLE::Adapter adapter);
  };

} // namespace simplejsble
} // namespace nitro
} // namespace margelo
