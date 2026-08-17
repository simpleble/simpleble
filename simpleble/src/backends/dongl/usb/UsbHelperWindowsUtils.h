#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace SimpleBLE {
namespace Dongl {
namespace USB {
namespace Detail {

inline wchar_t ascii_upper(wchar_t value) {
    if (value >= L'a' && value <= L'z') {
        return static_cast<wchar_t>(value - L'a' + L'A');
    }
    return value;
}

inline wchar_t hex_digit(unsigned int value) {
    return static_cast<wchar_t>(value < 10 ? L'0' + value : L'A' + value - 10);
}

inline std::wstring usb_hardware_id_fragment(uint16_t vendor_id, uint16_t product_id) {
    std::wstring fragment = L"VID_0000&PID_0000";
    for (std::size_t index = 0; index < 4; ++index) {
        const unsigned int shift = static_cast<unsigned int>((3 - index) * 4);
        fragment[4 + index] = hex_digit((vendor_id >> shift) & 0x0f);
        fragment[13 + index] = hex_digit((product_id >> shift) & 0x0f);
    }
    return fragment;
}

inline bool hardware_id_matches_usb_device(std::wstring_view hardware_id, uint16_t vendor_id, uint16_t product_id) {
    std::wstring normalized(hardware_id);
    for (wchar_t& value : normalized) {
        value = ascii_upper(value);
    }

    const std::wstring fragment = usb_hardware_id_fragment(vendor_id, product_id);
    std::size_t position = normalized.find(fragment);
    while (position != std::wstring::npos) {
        const bool starts_at_token = position == 0 || normalized[position - 1] == L'\\' ||
                                     normalized[position - 1] == L'&';
        const std::size_t end = position + fragment.size();
        const bool ends_at_token = end == normalized.size() || normalized[end] == L'&';
        if (starts_at_token && ends_at_token) {
            return true;
        }
        position = normalized.find(fragment, position + 1);
    }
    return false;
}

inline std::optional<std::string> windows_serial_path(std::wstring_view port_name) {
    if (port_name.size() < 4 || ascii_upper(port_name[0]) != L'C' || ascii_upper(port_name[1]) != L'O' ||
        ascii_upper(port_name[2]) != L'M') {
        return std::nullopt;
    }

    std::string path = "\\\\.\\COM";
    for (std::size_t index = 3; index < port_name.size(); ++index) {
        if (port_name[index] < L'0' || port_name[index] > L'9') {
            return std::nullopt;
        }
        path.push_back(static_cast<char>(port_name[index]));
    }
    return path;
}

}  // namespace Detail
}  // namespace USB
}  // namespace Dongl
}  // namespace SimpleBLE
