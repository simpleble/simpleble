#include "UsbHelperLinux.h"

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace SimpleBLE {
namespace Dongl {
namespace USB {

namespace {

bool read_hex_uint16(const std::filesystem::path& path, uint16_t& value) {
    std::ifstream stream(path);
    uint32_t parsed_value;
    if (!(stream >> std::hex >> parsed_value) || parsed_value > std::numeric_limits<uint16_t>::max()) {
        return false;
    }

    value = static_cast<uint16_t>(parsed_value);
    return true;
}

bool is_dongl_tty(const std::filesystem::path& tty_path) {
    std::error_code error;
    auto current_path = std::filesystem::canonical(tty_path / "device", error);
    if (error) {
        return false;
    }

    while (current_path != current_path.root_path()) {
        uint16_t vendor_id;
        uint16_t product_id;
        if (read_hex_uint16(current_path / "idVendor", vendor_id) &&
            read_hex_uint16(current_path / "idProduct", product_id)) {
            return vendor_id == UsbHelperImpl::DONGL_VENDOR_ID && product_id == UsbHelperImpl::DONGL_PRODUCT_ID;
        }

        current_path = current_path.parent_path();
    }

    return false;
}

}  // namespace

UsbHelperLinux::UsbHelperLinux(const std::string& device_path) : UsbHelperImpl(device_path) {
    if (!_open_serial_port()) {
        throw std::runtime_error("Failed to open serial port " + _device_path + ": " + std::strerror(errno));
    }

    try {
        _running = true;
        _thread = std::thread(&UsbHelperLinux::_run, this);
    } catch (...) {
        _running = false;
        _close_serial_port();
        throw;
    }
}

UsbHelperLinux::~UsbHelperLinux() {
    _running = false;
    if (_thread.joinable()) {
        _thread.join();
    }
    _close_serial_port();
}

void UsbHelperLinux::tx(const kvn::bytearray& data) {
    std::scoped_lock lock(_tx_mutex);
    size_t offset = 0;

    while (offset < data.size()) {
        const ssize_t bytes_written = write(_serial_fd, data.data() + offset, data.size() - offset);
        if (bytes_written > 0) {
            offset += static_cast<size_t>(bytes_written);
            continue;
        }

        if (bytes_written < 0 && errno == EINTR) {
            continue;
        }

        if (bytes_written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd descriptor{_serial_fd, POLLOUT, 0};
            int poll_result;
            do {
                poll_result = poll(&descriptor, 1, 1000);
            } while (poll_result < 0 && errno == EINTR);

            if (poll_result > 0) {
                continue;
            }
            if (poll_result == 0) {
                throw std::runtime_error("Timed out writing to serial port: " + _device_path);
            }
        }

        throw std::runtime_error("Failed to write to serial port " + _device_path + ": " + std::strerror(errno));
    }
}

void UsbHelperLinux::set_rx_callback(std::function<void(const kvn::bytearray&)> callback) {
    _rx_callback.load(callback);
}

std::vector<std::string> UsbHelperLinux::get_dongl_devices() {
    std::vector<std::string> dongl_devices;
    std::error_code error;

    for (const auto& entry : std::filesystem::directory_iterator("/sys/class/tty", error)) {
        if (is_dongl_tty(entry.path())) {
            const auto device_path = std::filesystem::path("/dev") / entry.path().filename();
            if (std::filesystem::exists(device_path, error) && !error) {
                dongl_devices.push_back(device_path.string());
            }
            error.clear();
        }
    }

    std::sort(dongl_devices.begin(), dongl_devices.end());
    return dongl_devices;
}

bool UsbHelperLinux::_open_serial_port() {
    _serial_fd = open(_device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (_serial_fd < 0) {
        return false;
    }

    try {
        _configure_serial_port();
    } catch (...) {
        _close_serial_port();
        throw;
    }

    return true;
}

void UsbHelperLinux::_close_serial_port() {
    if (_serial_fd >= 0) {
        close(_serial_fd);
        _serial_fd = -1;
    }
}

void UsbHelperLinux::_configure_serial_port() {
    termios tty;
    if (tcgetattr(_serial_fd, &tty) != 0) {
        throw std::runtime_error("Failed to get serial port attributes: " + std::string(std::strerror(errno)));
    }

    cfmakeraw(&tty);
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (cfsetispeed(&tty, B1000000) != 0 || cfsetospeed(&tty, B1000000) != 0) {
        throw std::runtime_error("Failed to set serial port speed: " + std::string(std::strerror(errno)));
    }
    if (tcsetattr(_serial_fd, TCSANOW, &tty) != 0) {
        throw std::runtime_error("Failed to set serial port attributes: " + std::string(std::strerror(errno)));
    }
    if (tcflush(_serial_fd, TCIOFLUSH) != 0) {
        throw std::runtime_error("Failed to flush serial port: " + std::string(std::strerror(errno)));
    }
}

void UsbHelperLinux::_run() {
    char buffer[256];

    while (_running) {
        pollfd descriptor{_serial_fd, POLLIN, 0};
        const int poll_result = poll(&descriptor, 1, 100);
        if (poll_result == 0) {
            continue;
        }
        if (poll_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Error polling serial port " << _device_path << ": " << std::strerror(errno) << std::endl;
            break;
        }
        if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            std::cerr << "Serial port disconnected: " << _device_path << std::endl;
            break;
        }
        if (!(descriptor.revents & POLLIN)) {
            continue;
        }

        const ssize_t bytes_read = read(_serial_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            _rx_callback(kvn::bytearray(buffer, static_cast<size_t>(bytes_read)));
        } else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            std::cerr << "Error reading from serial port " << _device_path << ": " << std::strerror(errno) << std::endl;
            break;
        }
    }
}

}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE
