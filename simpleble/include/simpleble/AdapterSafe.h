#pragma once

#include <simpleble/export.h>

#include <simpleble/Adapter.h>
#include <simpleble/PeripheralSafe.h>
#include <memory>
#include <optional>
#include <vector>

namespace SimpleBLE {

namespace Safe {

/**
 * Wrapper around the Adapter class that provides a noexcept interface.
 *
 * We use instances of this class directly and not through shared_ptr because
 * this is just a wrapper around the Adapter class, which is already managed by
 * shared_ptr.
 */
class SIMPLEBLE_EXPORT Adapter {
  public:
    Adapter() = default;
    virtual ~Adapter() = default;

    std::optional<std::string> identifier() const noexcept;
    std::optional<BluetoothAddress> address() noexcept;

    bool scan_start() noexcept;
    bool scan_stop() noexcept;
    bool scan_for(int timeout_ms) noexcept;
    std::optional<bool> scan_is_active() noexcept;
    std::optional<std::vector<SimpleBLE::Safe::Peripheral>> scan_get_results() noexcept;

    bool set_callback_on_scan_start(std::function<void()> on_scan_start) noexcept;
    bool set_callback_on_scan_stop(std::function<void()> on_scan_stop) noexcept;
    bool set_callback_on_scan_updated(std::function<void(SimpleBLE::Safe::Peripheral)> on_scan_updated) noexcept;
    bool set_callback_on_scan_found(std::function<void(SimpleBLE::Safe::Peripheral)> on_scan_found) noexcept;

    std::optional<std::vector<SimpleBLE::Safe::Peripheral>> get_paired_peripherals() noexcept;

    std::optional<bool> bluetooth_enabled() noexcept;
    static std::optional<std::vector<Adapter>> get_adapters() noexcept;

    /**
     * Get the underlying adapter object.
     */
    const std::shared_ptr<SimpleBLE::Adapter> underlying() const noexcept;

    /**
     * Cast to the underlying adapter object.
     */
    operator SimpleBLE::Adapter() const noexcept;

  protected:
    SimpleBLE::Adapter internal_;
};

}  // namespace Safe

}  // namespace SimpleBLE
