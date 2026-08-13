#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include "UsbHelperImpl.h"

namespace SimpleBLE {
namespace Dongl {
namespace USB {

class UsbHelperLinux : public UsbHelperImpl {
  public:
    UsbHelperLinux(const std::string& device_path);
    ~UsbHelperLinux();

    void tx(const kvn::bytearray& data);
    void set_rx_callback(std::function<void(const kvn::bytearray&)> callback);

    static std::vector<std::string> get_dongl_devices();

  private:
    void _run();
    bool _open_serial_port();
    void _close_serial_port();
    void _configure_serial_port();

    std::atomic_bool _running{false};
    std::thread _thread;
    std::mutex _tx_mutex;
    int _serial_fd{-1};
};

}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE
