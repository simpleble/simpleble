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

    /**
     * Get the backend of the operating system's Bluetooth stack.
     *
     * This is SimpleBluez on Linux, WinRT on Windows, CoreBluetooth on macOS and iOS, and Android on Android.
     */
    static Backend get_native();

    /**
     * Get the Dongl backend.
     *
     * NOTE: This works whether or not Config::Dongl::use_dongl_backend includes the Dongl backend in
     * get_backends().
     *
     * @throws Exception::OperationNotSupported if this build of SimpleBLE doesn't include the Dongl backend.
     */
    static Backend get_dongl();

    /**
     * Get the Simulator backend.
     *
     * NOTE: It only has adapters while a Simulation::Environment is active.
     *
     * @throws Exception::OperationNotSupported if this build of SimpleBLE doesn't include the Simulator backend.
     */
    static Backend get_simulator();

  protected:
    BackendBase* operator->();
    const BackendBase* operator->() const;

    std::shared_ptr<BackendBase> internal_;
};

}  // namespace SimpleBLE
