#include "UsbHelperWindows.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <setupapi.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "LoggingInternal.h"
#include "UsbHelperWindowsUtils.h"

namespace SimpleBLE {
namespace Dongl {
namespace USB {

namespace {

// GUID_DEVCLASS_PORTS from the Windows SDK's devguid.h.
constexpr GUID PORTS_DEVICE_CLASS = {0x4d36e978, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};
constexpr DWORD IO_WAIT_TIMEOUT_MS = 1000;
constexpr DWORD IO_POLL_INTERVAL_MS = 100;

class ScopedHandle {
  public:
    explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;
    ScopedHandle(ScopedHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) {
            close();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }
    ~ScopedHandle() { close(); }

    HANDLE get() const { return handle_; }

  private:
    void close() {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
            handle_ = nullptr;
        }
    }

    HANDLE handle_;
};

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

class OverlappedOperation {
  public:
    explicit OverlappedOperation(size_t buffer_size)
        : event_(CreateEventW(nullptr, TRUE, FALSE, nullptr)), buffer_(buffer_size) {
        reset();
    }

    bool valid() const { return event_.get() != nullptr; }
    HANDLE event() const { return event_.get(); }
    OVERLAPPED* overlapped() { return &overlapped_; }
    uint8_t* data() { return buffer_.data(); }
    size_t size() const { return buffer_.size(); }

    void reset() {
        overlapped_ = {};
        overlapped_.hEvent = event_.get();
        if (event_.get() != nullptr) {
            ResetEvent(event_.get());
        }
    }

  private:
    ScopedHandle event_;
    OVERLAPPED overlapped_{};
    std::vector<uint8_t> buffer_;
};

std::string windows_error_message(DWORD error);

bool cancel_and_wait(HANDLE serial_handle, OverlappedOperation& operation) {
    if (!CancelIoEx(serial_handle, operation.overlapped())) {
        const DWORD cancel_error = GetLastError();
        if (cancel_error != ERROR_NOT_FOUND) {
            SIMPLEBLE_LOG_WARN(fmt::format("Failed to cancel serial I/O: {}", windows_error_message(cancel_error)));
        }
    }
    return WaitForSingleObject(operation.event(), IO_WAIT_TIMEOUT_MS) == WAIT_OBJECT_0;
}

void defer_operation_cleanup(std::unique_ptr<OverlappedOperation> operation) noexcept {
    OverlappedOperation* pending_operation = operation.release();
    try {
        std::thread([pending_operation]() {
            DWORD wait_result;
            do {
                wait_result = WaitForSingleObject(pending_operation->event(), IO_WAIT_TIMEOUT_MS);
            } while (wait_result == WAIT_TIMEOUT);
            if (wait_result == WAIT_OBJECT_0) {
                delete pending_operation;
            }
        }).detach();
    } catch (...) {
        // The kernel can still access the OVERLAPPED structure and buffer. Leaking this
        // exceptional-path allocation is safer than freeing live I/O state.
    }
}

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
    {
        std::scoped_lock lock(_serial_mutex);
        if (_serial_handle != nullptr && !CancelIoEx(static_cast<HANDLE>(_serial_handle), nullptr)) {
            const DWORD cancel_error = GetLastError();
            if (cancel_error != ERROR_NOT_FOUND) {
                SIMPLEBLE_LOG_WARN(fmt::format("Failed to cancel serial I/O during shutdown for {}: {}", _device_path,
                                               windows_error_message(cancel_error)));
            }
        }
    }
    if (_thread.joinable()) {
        _thread.join();
    }

    std::scoped_lock lock(_serial_mutex);
    _close_serial_port();
}

void UsbHelperWindows::tx(const kvn::bytearray& data) {
    std::scoped_lock lock(_serial_mutex);
    if (!_running || _serial_handle == nullptr) {
        throw std::runtime_error("Serial port is not available: " + _device_path);
    }

    HANDLE serial_handle = static_cast<HANDLE>(_serial_handle);
    size_t offset = 0;

    while (offset < data.size()) {
        const size_t remaining = data.size() - offset;
        const DWORD requested = static_cast<DWORD>(
            std::min(remaining, static_cast<size_t>(std::numeric_limits<DWORD>::max())));
        auto operation = std::make_unique<OverlappedOperation>(requested);
        if (!operation->valid()) {
            const DWORD error = GetLastError();
            throw std::runtime_error("Failed to create serial port write event: " + windows_error_message(error));
        }
        std::memcpy(operation->data(), data.data() + offset, requested);

        DWORD bytes_written = 0;
        if (!WriteFile(serial_handle, operation->data(), requested, &bytes_written, operation->overlapped())) {
            const DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING) {
                _running = false;
                CancelIoEx(serial_handle, nullptr);
                throw std::runtime_error("Failed to write to serial port " + _device_path + ": " +
                                         windows_error_message(error));
            }

            const DWORD wait_result = WaitForSingleObject(operation->event(), IO_WAIT_TIMEOUT_MS);
            if (wait_result == WAIT_TIMEOUT) {
                _running = false;
                if (!cancel_and_wait(serial_handle, *operation)) {
                    SIMPLEBLE_LOG_ERROR(
                        fmt::format("Cancellation timed out while writing to {}; deferring I/O cleanup", _device_path));
                    defer_operation_cleanup(std::move(operation));
                }
                throw std::runtime_error("Timed out writing to serial port: " + _device_path);
            }
            if (wait_result != WAIT_OBJECT_0) {
                const DWORD wait_error = wait_result == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
                _running = false;
                if (!cancel_and_wait(serial_handle, *operation)) {
                    SIMPLEBLE_LOG_ERROR(
                        fmt::format("Cancellation timed out after a write wait failure on {}; deferring I/O cleanup",
                                    _device_path));
                    defer_operation_cleanup(std::move(operation));
                }
                throw std::runtime_error("Failed to write to serial port " + _device_path + ": " +
                                         windows_error_message(wait_error));
            }
            if (!GetOverlappedResult(serial_handle, operation->overlapped(), &bytes_written, FALSE)) {
                const DWORD result_error = GetLastError();
                _running = false;
                CancelIoEx(serial_handle, nullptr);
                throw std::runtime_error("Failed to write to serial port " + _device_path + ": " +
                                         windows_error_message(result_error));
            }
        }
        if (bytes_written == 0) {
            _running = false;
            CancelIoEx(serial_handle, nullptr);
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
    // Match the asserted DTR state used when the POSIX helpers open this CDC transport.
    // Configure it once before I/O starts so it is never pulsed during normal operation.
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
        throw std::runtime_error("Failed to flush serial port " + _device_path + ": " + windows_error_message(error));
    }
}

void UsbHelperWindows::_run() {
    auto operation = std::make_unique<OverlappedOperation>(256);
    if (!operation->valid()) {
        const DWORD error = GetLastError();
        _running = false;
        SIMPLEBLE_LOG_ERROR(fmt::format("Failed to create serial port read event: {}", windows_error_message(error)));
        return;
    }

    while (_running) {
        operation->reset();

        HANDLE serial_handle;
        DWORD bytes_read = 0;
        bool read_complete;
        DWORD read_error = ERROR_SUCCESS;
        {
            std::scoped_lock lock(_serial_mutex);
            if (!_running || _serial_handle == nullptr) {
                break;
            }
            serial_handle = static_cast<HANDLE>(_serial_handle);
            read_complete = ReadFile(serial_handle, operation->data(), static_cast<DWORD>(operation->size()),
                                     &bytes_read, operation->overlapped()) != FALSE;
            if (!read_complete) {
                read_error = GetLastError();
            }
        }

        if (!read_complete) {
            if (read_error != ERROR_IO_PENDING) {
                _running = false;
                SIMPLEBLE_LOG_ERROR(fmt::format("Error reading from serial port {}: {}", _device_path,
                                                windows_error_message(read_error)));
                break;
            }

            DWORD wait_result = WAIT_TIMEOUT;
            while (_running && wait_result == WAIT_TIMEOUT) {
                wait_result = WaitForSingleObject(operation->event(), IO_POLL_INTERVAL_MS);
            }

            if (!_running) {
                bool cancellation_completed = true;
                {
                    std::scoped_lock lock(_serial_mutex);
                    if (_serial_handle != nullptr && static_cast<HANDLE>(_serial_handle) == serial_handle) {
                        cancellation_completed = cancel_and_wait(serial_handle, *operation);
                    }
                }
                if (!cancellation_completed) {
                    SIMPLEBLE_LOG_ERROR(fmt::format(
                        "Cancellation timed out while reading from {}; deferring I/O cleanup", _device_path));
                    defer_operation_cleanup(std::move(operation));
                }
                break;
            }

            if (wait_result != WAIT_OBJECT_0) {
                const DWORD wait_error = wait_result == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
                _running = false;
                bool cancellation_completed;
                {
                    std::scoped_lock lock(_serial_mutex);
                    cancellation_completed = cancel_and_wait(serial_handle, *operation);
                }
                if (!cancellation_completed) {
                    SIMPLEBLE_LOG_ERROR(fmt::format(
                        "Cancellation timed out after a read wait failure on {}; deferring I/O cleanup", _device_path));
                    defer_operation_cleanup(std::move(operation));
                }
                SIMPLEBLE_LOG_ERROR(fmt::format("Failed waiting for serial data on {}: {}", _device_path,
                                                windows_error_message(wait_error)));
                break;
            }

            if (!GetOverlappedResult(serial_handle, operation->overlapped(), &bytes_read, FALSE)) {
                const DWORD result_error = GetLastError();
                _running = false;
                SIMPLEBLE_LOG_ERROR(fmt::format("Error reading from serial port {}: {}", _device_path,
                                                windows_error_message(result_error)));
                break;
            }
        }
        if (bytes_read > 0) {
            _rx_callback(kvn::bytearray(operation->data(), static_cast<size_t>(bytes_read)));
        }
    }
}

}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE
