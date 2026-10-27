#include "UsbHelperLinux.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

#include <simpleble/Exceptions.h>

#include "LoggingInternal.h"

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

std::string read_line(const std::filesystem::path& path) {
    std::ifstream stream(path);
    std::string line;
    std::getline(stream, line);
    return line;
}

// Returns the sysfs directory of the USB device behind a tty, if that device is a Dongl.
std::optional<std::filesystem::path> dongl_usb_device(const std::filesystem::path& tty_path) {
    std::error_code error;
    auto current_path = std::filesystem::canonical(tty_path / "device", error);
    if (error) {
        return std::nullopt;
    }

    while (current_path != current_path.root_path()) {
        uint16_t vendor_id;
        uint16_t product_id;
        if (read_hex_uint16(current_path / "idVendor", vendor_id) &&
            read_hex_uint16(current_path / "idProduct", product_id)) {
            if (vendor_id == UsbHelperImpl::DONGL_VENDOR_ID && product_id == UsbHelperImpl::DONGL_PRODUCT_ID) {
                return current_path;
            }
            return std::nullopt;
        }

        current_path = current_path.parent_path();
    }

    return std::nullopt;
}

}  // namespace

UsbHelperLinux::UsbHelperLinux(const std::string& device_path) : UsbHelperImpl(device_path) {
    if (!_open_serial_port()) {
        if (errno == EBUSY) throw Exception::AdapterInUse(_device_path);
        throw Exception::OperationFailed("Failed to open serial port " + _device_path + ": " + std::strerror(errno));
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

    std::scoped_lock lock(_serial_mutex);
    _close_serial_port();
}

void UsbHelperLinux::tx(const kvn::bytearray& data) {
    std::scoped_lock lock(_serial_mutex);
    if (!_running || _serial_fd < 0) {
        throw Exception::OperationFailed("Serial port is not available: " + _device_path);
    }

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

            if (poll_result > 0 && (descriptor.revents & POLLOUT)) {
                continue;
            }
            if (poll_result == 0) {
                _running = false;
                _close_serial_port();
                throw Exception::OperationFailed("Timed out writing to serial port: " + _device_path);
            }

            _running = false;
            _close_serial_port();
            throw Exception::OperationFailed("Serial port became unavailable while writing: " + _device_path);
        }

        const int write_error = bytes_written == 0 ? EIO : errno;
        _running = false;
        _close_serial_port();
        throw Exception::OperationFailed("Failed to write to serial port " + _device_path + ": " +
                                         std::strerror(write_error));
    }
}

void UsbHelperLinux::set_rx_callback(std::function<void(const kvn::bytearray&)> callback) {
    _rx_callback.load(callback);
}

std::vector<DonglDevice> UsbHelperLinux::get_dongl_devices() {
    std::vector<DonglDevice> dongl_devices;
    std::error_code error;

    for (const auto& entry : std::filesystem::directory_iterator("/sys/class/tty", error)) {
        const auto usb_device = dongl_usb_device(entry.path());
        if (!usb_device.has_value()) {
            continue;
        }

        const auto device_path = std::filesystem::path("/dev") / entry.path().filename();
        if (std::filesystem::exists(device_path, error) && !error) {
            dongl_devices.push_back(
                {device_path.string(), read_line(*usb_device / "serial"), read_line(*usb_device / "product")});
        }
        error.clear();
    }

    std::sort(dongl_devices.begin(), dongl_devices.end(),
              [](const DonglDevice& a, const DonglDevice& b) { return a.path < b.path; });
    return dongl_devices;
}

bool UsbHelperLinux::_open_serial_port() {
    _serial_fd = open(_device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (_serial_fd < 0) {
        return false;
    }

    // Prevent other processes from using the port while we hold it, as they would take bytes meant for us.
    // NOTE: TIOCEXCL doesn't stop root or a process opening the port at the same time, so we also take
    // a lock that other SimpleBLE processes will respect.
    if (flock(_serial_fd, LOCK_EX | LOCK_NB) != 0) {
        const int lock_error = errno == EWOULDBLOCK ? EBUSY : errno;
        _close_serial_port();
        errno = lock_error;
        return false;
    }
    if (ioctl(_serial_fd, TIOCEXCL) == -1) {
        SIMPLEBLE_LOG_WARN(fmt::format("Failed to get exclusive access to {}: {}", _device_path, std::strerror(errno)));
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
        throw Exception::OperationFailed("Failed to get serial port attributes: " + std::string(std::strerror(errno)));
    }

    cfmakeraw(&tty);
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (cfsetispeed(&tty, B1000000) != 0 || cfsetospeed(&tty, B1000000) != 0) {
        throw Exception::OperationFailed("Failed to set serial port speed: " + std::string(std::strerror(errno)));
    }
    if (tcsetattr(_serial_fd, TCSANOW, &tty) != 0) {
        throw Exception::OperationFailed("Failed to set serial port attributes: " + std::string(std::strerror(errno)));
    }
    if (tcflush(_serial_fd, TCIOFLUSH) != 0) {
        throw Exception::OperationFailed("Failed to flush serial port: " + std::string(std::strerror(errno)));
    }
}

void UsbHelperLinux::_run() {
    char buffer[256];

    while (_running) {
        int serial_fd;
        {
            std::scoped_lock lock(_serial_mutex);
            if (!_running || _serial_fd < 0) {
                break;
            }
            serial_fd = _serial_fd;
        }

        pollfd descriptor{serial_fd, POLLIN, 0};
        const int poll_result = poll(&descriptor, 1, 100);
        if (poll_result == 0) {
            continue;
        }
        if (poll_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            const int poll_error = errno;
            _running = false;
            SIMPLEBLE_LOG_ERROR(
                fmt::format("Error polling serial port {}: {}", _device_path, std::strerror(poll_error)));
            _lost_callback();
            break;
        }
        if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            _running = false;
            SIMPLEBLE_LOG_WARN(fmt::format("Serial port disconnected: {}", _device_path));
            _lost_callback();
            break;
        }
        if (!(descriptor.revents & POLLIN)) {
            continue;
        }

        ssize_t bytes_read;
        int read_error = 0;
        {
            std::scoped_lock lock(_serial_mutex);
            if (!_running || _serial_fd != serial_fd) {
                break;
            }
            bytes_read = read(_serial_fd, buffer, sizeof(buffer));
            if (bytes_read < 0) {
                read_error = errno;
            }
        }

        if (bytes_read > 0) {
            _rx_callback(kvn::bytearray(buffer, static_cast<size_t>(bytes_read)));
        } else if (bytes_read < 0 && read_error != EAGAIN && read_error != EWOULDBLOCK && read_error != EINTR) {
            _running = false;
            SIMPLEBLE_LOG_ERROR(
                fmt::format("Error reading from serial port {}: {}", _device_path, std::strerror(read_error)));
            _lost_callback();
            break;
        }
    }
}

}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE
