#include "UsbHelperWindows.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <setupapi.h>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "LoggingInternal.h"
#include "UsbHelperWindowsUtils.h"

namespace SimpleBLE {
namespace Dongl {
namespace USB {

namespace {

// GUID_DEVCLASS_PORTS from the Windows SDK's devguid.h.
constexpr GUID PORTS_DEVICE_CLASS = {0x4d36e978, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};
constexpr DWORD READ_TIMEOUT_MS = 100;
constexpr DWORD WRITE_TIMEOUT_MS = 1000;

class ScopedDeviceInfoSet {
  public:
    explicit ScopedDeviceInfoSet(HDEVINFO handle) : handle_(handle) {}
    ScopedDeviceInfoSet(const ScopedDeviceInfoSet&) = delete;
    ScopedDeviceInfoSet& operator=(const ScopedDeviceInfoSet&) = delete;
    ~ScopedDeviceInfoSet() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            SetupDiDestroyDeviceInfoList(handle_);
        }
    }

    HDEVINFO get() const { return handle_; }

  private:
    HDEVINFO handle_;
};

class ScopedRegistryKey {
  public:
    explicit ScopedRegistryKey(HKEY key) : key_(key) {}
    ScopedRegistryKey(const ScopedRegistryKey&) = delete;
    ScopedRegistryKey& operator=(const ScopedRegistryKey&) = delete;
    ~ScopedRegistryKey() {
        if (key_ != INVALID_HANDLE_VALUE) {
            RegCloseKey(key_);
        }
    }

    HKEY get() const { return key_; }

  private:
    HKEY key_;
};

std::string windows_error_message(DWORD error) {
    char* message = nullptr;
    const DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0,
        reinterpret_cast<char*>(&message), 0, nullptr);

    std::string result;
    if (size != 0 && message != nullptr) {
        result.assign(message, size);
        while (!result.empty() && (result.back() == '\r' || result.back() == '\n')) {
            result.pop_back();
        }
    } else {
        result = "Windows error " + std::to_string(error);
    }

    if (message != nullptr) {
        LocalFree(message);
    }
    return result;
}

bool is_dongl_device(HDEVINFO device_info_set, SP_DEVINFO_DATA& device_info) {
    DWORD property_type = 0;
    DWORD required_size = 0;
    SetupDiGetDeviceRegistryPropertyW(device_info_set, &device_info, SPDRP_HARDWAREID, &property_type, nullptr, 0,
                                      &required_size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || required_size == 0) {
        return false;
    }

    std::vector<BYTE> buffer(required_size);
    if (!SetupDiGetDeviceRegistryPropertyW(device_info_set, &device_info, SPDRP_HARDWAREID, &property_type,
                                           buffer.data(), static_cast<DWORD>(buffer.size()), nullptr) ||
        (property_type != REG_SZ && property_type != REG_MULTI_SZ)) {
        return false;
    }

    const wchar_t* current = reinterpret_cast<const wchar_t*>(buffer.data());
    const wchar_t* const end = current + buffer.size() / sizeof(wchar_t);

    while (current < end && *current != L'\0') {
        const wchar_t* terminator = std::find(current, end, L'\0');
        if (Detail::hardware_id_matches_usb_device(
                std::wstring_view(current, static_cast<size_t>(terminator - current)), UsbHelperImpl::DONGL_VENDOR_ID,
                UsbHelperImpl::DONGL_PRODUCT_ID)) {
            return true;
        }
        if (terminator == end) {
            break;
        }
        current = terminator + 1;
    }

    return false;
}

std::wstring get_port_name(HDEVINFO device_info_set, SP_DEVINFO_DATA& device_info) {
    ScopedRegistryKey key(
        SetupDiOpenDevRegKey(device_info_set, &device_info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE));
    if (key.get() == INVALID_HANDLE_VALUE) {
        return {};
    }

    DWORD property_type = 0;
    DWORD required_size = 0;
    LSTATUS status = RegQueryValueExW(key.get(), L"PortName", nullptr, &property_type, nullptr, &required_size);
    if (status != ERROR_SUCCESS || property_type != REG_SZ || required_size < sizeof(wchar_t)) {
        return {};
    }

    std::vector<wchar_t> buffer(required_size / sizeof(wchar_t) + 1, L'\0');
    status = RegQueryValueExW(key.get(), L"PortName", nullptr, &property_type, reinterpret_cast<BYTE*>(buffer.data()),
                              &required_size);
    if (status != ERROR_SUCCESS || property_type != REG_SZ) {
        return {};
    }

    return buffer.data();
}

}  // namespace

UsbHelperWindows::UsbHelperWindows(const std::string& device_path) : UsbHelperImpl(device_path) {
    if (!_open_serial_port()) {
        const DWORD error = GetLastError();
        throw std::runtime_error("Failed to open serial port " + _device_path + ": " + windows_error_message(error));
    }

    try {
        _running = true;
        _thread = std::thread(&UsbHelperWindows::_run, this);
    } catch (...) {
        _running = false;
        _close_serial_port();
        throw;
    }
}

UsbHelperWindows::~UsbHelperWindows() {
    _running = false;
    if (_thread.joinable()) {
        _thread.join();
    }

    std::scoped_lock tx_lock(_tx_mutex);
    std::scoped_lock serial_lock(_serial_mutex);
    _close_serial_port();
}

void UsbHelperWindows::tx(const kvn::bytearray& data) {
    std::scoped_lock tx_lock(_tx_mutex);

    HANDLE serial_handle;
    {
        std::scoped_lock serial_lock(_serial_mutex);
        if (!_running || _serial_handle == nullptr) {
            throw std::runtime_error("Serial port is not available: " + _device_path);
        }
        serial_handle = static_cast<HANDLE>(_serial_handle);
    }

    size_t offset = 0;

    while (offset < data.size()) {
        const size_t remaining = data.size() - offset;
        const DWORD requested = static_cast<DWORD>(
            std::min(remaining, static_cast<size_t>(std::numeric_limits<DWORD>::max())));

        DWORD bytes_written = 0;
        if (!WriteFile(serial_handle, data.data() + offset, requested, &bytes_written, nullptr)) {
            const DWORD error = GetLastError();
            throw std::runtime_error("Failed to write to serial port " + _device_path + ": " +
                                     windows_error_message(error));
        }
        if (bytes_written == 0) {
            throw std::runtime_error("Failed to write to serial port " + _device_path + ": no bytes were written");
        }
        offset += bytes_written;
    }
}

void UsbHelperWindows::set_rx_callback(std::function<void(const kvn::bytearray&)> callback) {
    _rx_callback.load(callback);
}

std::vector<std::string> UsbHelperWindows::get_dongl_devices() {
    std::vector<std::string> dongl_devices;
    ScopedDeviceInfoSet device_info_set(SetupDiGetClassDevsW(&PORTS_DEVICE_CLASS, nullptr, nullptr, DIGCF_PRESENT));
    if (device_info_set.get() == INVALID_HANDLE_VALUE) {
        return dongl_devices;
    }

    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA device_info{};
        device_info.cbSize = sizeof(device_info);
        if (!SetupDiEnumDeviceInfo(device_info_set.get(), index, &device_info)) {
            break;
        }
        if (!is_dongl_device(device_info_set.get(), device_info)) {
            continue;
        }

        const auto device_path = Detail::windows_serial_path(get_port_name(device_info_set.get(), device_info));
        if (!device_path.has_value()) {
            continue;
        }
        dongl_devices.push_back(*device_path);
    }

    std::sort(dongl_devices.begin(), dongl_devices.end());
    dongl_devices.erase(std::unique(dongl_devices.begin(), dongl_devices.end()), dongl_devices.end());
    return dongl_devices;
}

bool UsbHelperWindows::_open_serial_port() {
    HANDLE serial_handle = CreateFileA(_device_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL, nullptr);
    if (serial_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    _serial_handle = serial_handle;
    try {
        _configure_serial_port();
    } catch (...) {
        _close_serial_port();
        throw;
    }
    return true;
}

void UsbHelperWindows::_close_serial_port() {
    if (_serial_handle != nullptr) {
        CloseHandle(static_cast<HANDLE>(_serial_handle));
        _serial_handle = nullptr;
    }
}

void UsbHelperWindows::_configure_serial_port() {
    HANDLE serial_handle = static_cast<HANDLE>(_serial_handle);
    DCB config{};
    config.DCBlength = sizeof(config);
    if (!GetCommState(serial_handle, &config)) {
        const DWORD error = GetLastError();
        throw std::runtime_error("Failed to get serial port attributes for " + _device_path + ": " +
                                 windows_error_message(error));
    }

    config.BaudRate = 1000000;
    config.ByteSize = 8;
    config.Parity = NOPARITY;
    config.StopBits = ONESTOPBIT;
    config.fBinary = TRUE;
    config.fParity = FALSE;
    config.fOutxCtsFlow = FALSE;
    config.fOutxDsrFlow = FALSE;
    // The tested Dongl N-Series does not answer protocol commands unless DTR remains asserted.
    config.fDtrControl = DTR_CONTROL_ENABLE;
    config.fDsrSensitivity = FALSE;
    config.fTXContinueOnXoff = TRUE;
    config.fOutX = FALSE;
    config.fInX = FALSE;
    config.fErrorChar = FALSE;
    config.fNull = FALSE;
    config.fAbortOnError = FALSE;
    config.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(serial_handle, &config)) {
        const DWORD error = GetLastError();
        throw std::runtime_error("Failed to set serial port attributes for " + _device_path + ": " +
                                 windows_error_message(error));
    }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = READ_TIMEOUT_MS;
    timeouts.WriteTotalTimeoutConstant = WRITE_TIMEOUT_MS;
    if (!SetCommTimeouts(serial_handle, &timeouts)) {
        const DWORD error = GetLastError();
        throw std::runtime_error("Failed to set serial port timeouts for " + _device_path + ": " +
                                 windows_error_message(error));
    }

    if (!PurgeComm(serial_handle, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        const DWORD error = GetLastError();
        throw std::runtime_error("Failed to flush serial port " + _device_path + ": " + windows_error_message(error));
    }
}

void UsbHelperWindows::_run() {
    char buffer[256];

    while (_running) {
        HANDLE serial_handle;
        {
            std::scoped_lock serial_lock(_serial_mutex);
            if (!_running || _serial_handle == nullptr) {
                break;
            }
            serial_handle = static_cast<HANDLE>(_serial_handle);
        }

        DWORD bytes_read = 0;
        if (!ReadFile(serial_handle, buffer, sizeof(buffer), &bytes_read, nullptr)) {
            const DWORD read_error = GetLastError();
            _running = false;
            SIMPLEBLE_LOG_ERROR(
                fmt::format("Error reading from serial port {}: {}", _device_path, windows_error_message(read_error)));
            break;
        }
        if (bytes_read > 0) {
            _rx_callback(kvn::bytearray(buffer, static_cast<size_t>(bytes_read)));
        }
    }
}

}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE
