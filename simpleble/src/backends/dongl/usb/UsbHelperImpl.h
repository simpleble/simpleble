#pragma once

#include <kvn/kvn_bytearray.h>
#include <kvn/kvn_safe_callback.hpp>
#include <string>

namespace SimpleBLE {
namespace Dongl {
namespace USB {

// A Dongl found on the system, described without opening its port.
struct DonglDevice {
    std::string path;     // Serial port to open, such as /dev/ttyACM0 or \\.\COM6.
    std::string serial;   // USB serial number, which is the Dongl's Bluetooth address without separators.
    std::string product;  // USB product name, such as "Dongl N-Series".
};

class UsbHelperImpl {
  public:
    UsbHelperImpl(const std::string& device_path) : _device_path(device_path) {}
    virtual ~UsbHelperImpl() = default;

    virtual void tx(const kvn::bytearray& data) = 0;
    virtual void set_rx_callback(std::function<void(const kvn::bytearray&)> callback) = 0;

    // Called when the port stops working on its own, for example when the Dongl gets unplugged.
    void set_lost_callback(std::function<void()> callback) { _lost_callback.load(std::move(callback)); }

    static const uint16_t DONGL_VENDOR_ID = 0x3918;
    static const uint16_t DONGL_PRODUCT_ID = 0x0001;

  protected:
    std::string _device_path;
    kvn::safe_callback<void(const kvn::bytearray&)> _rx_callback;
    kvn::safe_callback<void()> _lost_callback;
};

}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE