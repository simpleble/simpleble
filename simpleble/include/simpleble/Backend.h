#pragma once

#include <memory>
#include <string>
#include <vector>

#include <simpleble/Adapter.h>
#include <simpleble/export.h>

namespace SimpleBLE {

class BackendBase;

class SIMPLEBLE_EXPORT Backend {
    friend class Adapter;
  public:
    Backend() = default;
    virtual ~Backend() = default;

    bool initialized() const;

    /**
     * Get a list of all available adapters from this backend.
     */
    std::vector<Adapter> adapters();

    /**
     * Check if Bluetooth is enabled for this backend.
     */
    bool bluetooth_enabled();

    /**
     * Get the identifier of the backend.
     */
    std::string identifier() const noexcept;

    /**
     * Get all available backends.
     *
     * This will cause backends to be instantiated/initialized.
     */
    static std::vector<Backend> get_backends();

  protected:
    BackendBase* operator->();
    const BackendBase* operator->() const;

    std::shared_ptr<BackendBase> internal_;
};

}  // namespace SimpleBLE
