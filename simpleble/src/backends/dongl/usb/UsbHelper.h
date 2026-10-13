#pragma once

#include <vector>
#include <string>
#include <memory>

#include <kvn/kvn_safe_callback.hpp>
#include <kvn/kvn_bytearray.h>

#include "UsbHelperImpl.h"

namespace SimpleBLE {
namespace Dongl {
namespace USB {

class UsbHelper {
  public:
    UsbHelper(const std::string& device_path);
    ~UsbHelper();

    void tx(const kvn::bytearray& data);
    void set_rx_callback(std::function<void(const kvn::bytearray&)> callback);
    void set_lost_callback(std::function<void()> callback);

    static std::vector<DonglDevice> get_dongl_devices();

  protected:
    std::unique_ptr<UsbHelperImpl> _impl;
};

}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE
