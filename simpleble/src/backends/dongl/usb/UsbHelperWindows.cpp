#include "UsbHelperWindows.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <setupapi.h>

#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace SimpleBLE {
namespace Dongl {
namespace USB {

namespace {

constexpr GUID PORTS_DEVICE_CLASS = {0x4d36e978,
                                     0xe325,
                                     0x11ce,
                                     {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};

class ScopedHandle {
  public:
    explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
    ~ScopedHandle() {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
    }

    HANDLE get() const { return handle_; }

  private:
    HANDLE handle_;
};

std::string windows_error_message(DWORD error) {
    char* message = nullptr;
    const DWORD size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                          FORMAT_MESSAGE_IGNORE_INSERTS,
                                      nullptr, error, 0, reinterpret_cast<char*>(&message), 0, nullptr);

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

    wchar_t dongl_id_buffer[18];
    if (std::swprintf(dongl_id_buffer, sizeof(dongl_id_buffer) / sizeof(dongl_id_buffer[0]),
                      L"VID_%04X&PID_%04X", static_cast<unsigned int>(UsbHelperImpl::DONGL_VENDOR_ID),
                      static_cast<unsigned int>(UsbHelperImpl::DONGL_PRODUCT_ID)) < 0) {
        return false;
    }

    const wchar_t* current = reinterpret_cast<const wchar_t*>(buffer.data());
    const wchar_t* const end = current + buffer.size() / sizeof(wchar_t);
    const std::wstring dongl_id(dongl_id_buffer);

    while (current < end && *current != L'\0') {
        const wchar_t* terminator = std::find(current, end, L'\0');
        std::wstring hardware_id(current, terminator);
        std::transform(hardware_id.begin(), hardware_id.end(), hardware_id.begin(),
                       [](wchar_t value) { return static_cast<wchar_t>(std::towupper(value)); });
        if (hardware_id.find(dongl_id) != std::wstring::npos) {
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
    HKEY key = SetupDiOpenDevRegKey(device_info_set, &device_info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
    if (key == INVALID_HANDLE_VALUE) {
        return {};
    }

    DWORD property_type = 0;
    DWORD required_size = 0;
    LSTATUS status = RegQueryValueExW(key, L"PortName", nullptr, &property_type, nullptr, &required_size);
    if (status != ERROR_SUCCESS || property_type != REG_SZ || required_size < sizeof(wchar_t)) {
        RegCloseKey(key);
        return {};
    }

    std::vector<wchar_t> buffer(required_size / sizeof(wchar_t) + 1, L'\0');
    status = RegQueryValueExW(key, L"PortName", nullptr, &property_type, reinterpret_cast<BYTE*>(buffer.data()),
                              &required_size);
    RegCloseKey(key);
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
    _close_serial_port();
}

void UsbHelperWindows::tx(const kvn::bytearray& data) {
    std::scoped_lock lock(_tx_mutex);
    HANDLE serial_handle = static_cast<HANDLE>(_serial_handle);
    ScopedHandle write_event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (write_event.get() == nullptr) {
        const DWORD error = GetLastError();
        throw std::runtime_error("Failed to create serial port write event: " + windows_error_message(error));
    }

    size_t offset = 0;

    while (offset < data.size()) {
        const size_t remaining = data.size() - offset;
        const DWORD requested = static_cast<DWORD>(
            std::min(remaining, static_cast<size_t>(std::numeric_limits<DWORD>::max())));
        OVERLAPPED operation{};
        operation.hEvent = write_event.get();
        ResetEvent(write_event.get());

        DWORD bytes_written = 0;
        if (!WriteFile(serial_handle, data.data() + offset, requested, &bytes_written, &operation)) {
            const DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING) {
                throw std::runtime_error("Failed to write to serial port " + _device_path + ": " +
                                         windows_error_message(error));
            }

            const DWORD wait_result = WaitForSingleObject(write_event.get(), 1000);
            if (wait_result == WAIT_TIMEOUT) {
                CancelIoEx(serial_handle, &operation);
                WaitForSingleObject(write_event.get(), INFINITE);
                throw std::runtime_error("Timed out writing to serial port: " + _device_path);
            }
            if (wait_result != WAIT_OBJECT_0) {
                const DWORD wait_error = GetLastError();
                CancelIoEx(serial_handle, &operation);
                WaitForSingleObject(write_event.get(), INFINITE);
                throw std::runtime_error("Failed to write to serial port " + _device_path + ": " +
                                         windows_error_message(wait_error));
            }
            if (!GetOverlappedResult(serial_handle, &operation, &bytes_written, FALSE)) {
                const DWORD result_error = GetLastError();
                throw std::runtime_error("Failed to write to serial port " + _device_path + ": " +
                                         windows_error_message(result_error));
            }
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
    HDEVINFO device_info_set =
        SetupDiGetClassDevsW(&PORTS_DEVICE_CLASS, nullptr, nullptr, DIGCF_PRESENT);
    if (device_info_set == INVALID_HANDLE_VALUE) {
        return dongl_devices;
    }

    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA device_info{};
        device_info.cbSize = sizeof(device_info);
        if (!SetupDiEnumDeviceInfo(device_info_set, index, &device_info)) {
            break;
        }
        if (!is_dongl_device(device_info_set, device_info)) {
            continue;
        }

        const std::wstring port_name = get_port_name(device_info_set, device_info);
        if (port_name.size() < 4 || port_name.compare(0, 3, L"COM") != 0 ||
            !std::all_of(port_name.begin() + 3, port_name.end(),
                         [](wchar_t value) { return value >= L'0' && value <= L'9'; })) {
            continue;
        }

        std::string port_name_ascii;
        port_name_ascii.reserve(port_name.size());
        std::transform(port_name.begin(), port_name.end(), std::back_inserter(port_name_ascii),
                       [](wchar_t value) { return static_cast<char>(value); });
        dongl_devices.push_back("\\\\.\\" + port_name_ascii);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);
    std::sort(dongl_devices.begin(), dongl_devices.end());
    dongl_devices.erase(std::unique(dongl_devices.begin(), dongl_devices.end()), dongl_devices.end());
    return dongl_devices;
}

bool UsbHelperWindows::_open_serial_port() {
    HANDLE serial_handle = CreateFileA(_device_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
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
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutConstant = 1000;
    if (!SetCommTimeouts(serial_handle, &timeouts)) {
        const DWORD error = GetLastError();
        throw std::runtime_error("Failed to set serial port timeouts for " + _device_path + ": " +
                                 windows_error_message(error));
    }

    if (!PurgeComm(serial_handle, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        const DWORD error = GetLastError();
        throw std::runtime_error("Failed to flush serial port " + _device_path + ": " +
                                 windows_error_message(error));
    }
}

void UsbHelperWindows::_run() {
    char buffer[256];
    HANDLE serial_handle = static_cast<HANDLE>(_serial_handle);
    ScopedHandle read_event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (read_event.get() == nullptr) {
        const DWORD error = GetLastError();
        std::cerr << "Failed to create serial port read event: " << windows_error_message(error) << std::endl;
        return;
    }

    while (_running) {
        OVERLAPPED operation{};
        operation.hEvent = read_event.get();
        ResetEvent(read_event.get());

        DWORD bytes_read = 0;
        bool read_complete = ReadFile(serial_handle, buffer, sizeof(buffer), &bytes_read, &operation) != FALSE;
        if (!read_complete) {
            const DWORD read_error = GetLastError();
            if (read_error != ERROR_IO_PENDING) {
                if (_running) {
                    std::cerr << "Error reading from serial port " << _device_path << ": "
                              << windows_error_message(read_error) << std::endl;
                }
                break;
            }

            DWORD wait_result = WAIT_TIMEOUT;
            while (_running && wait_result == WAIT_TIMEOUT) {
                wait_result = WaitForSingleObject(read_event.get(), 100);
            }

            if (!_running) {
                CancelIoEx(serial_handle, &operation);
                WaitForSingleObject(read_event.get(), INFINITE);
                break;
            }
            if (wait_result != WAIT_OBJECT_0 ||
                !GetOverlappedResult(serial_handle, &operation, &bytes_read, FALSE)) {
                const DWORD result_error = GetLastError();
                if (_running) {
                    std::cerr << "Error reading from serial port " << _device_path << ": "
                              << windows_error_message(result_error) << std::endl;
                }
                break;
            }
        }
        if (bytes_read > 0) {
            _rx_callback(kvn::bytearray(buffer, static_cast<size_t>(bytes_read)));
        }
    }
}

}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE
