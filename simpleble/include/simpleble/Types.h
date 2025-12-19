#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <string_view>
#include <stdexcept>
#include "kvn/kvn_bytearray.h"

/**
 * @file Types.h
 * @brief Defines types and enumerations used throughout the SimpleBLE library.
 */

namespace SimpleBLE {

/**
 * @class bluetoothAddress
 * @brief A class to handle Bluetooth addresses.
 */
class bluetoothAddress {
  public:
    /**
     * @brief Default constructor.
     */
    bluetoothAddress() = default;

    /**
     * @brief Constructs a bluetoothAddress from a C-style string.
     * @param s A C-style string representing the address.
     * @throws std::invalid_argument If the pointer is null.
     */
    bluetoothAddress(const char* s)
        : raw_(s ? s : throw std::invalid_argument("Bluetooth address pointer cannot be null")) {}

    /**
     * @brief Constructs a bluetoothAddress from a std::string.
     * @param s A string representing the address.
     */
    bluetoothAddress(const std::string& s) : raw_(s) {}

    /**
     * @brief Constructs a bluetoothAddress from a std::string (move).
     * @param s A string representing the address.
     */
    bluetoothAddress(std::string&& s) : raw_(std::move(s)) {}

    /**
     * @brief Constructs a bluetoothAddress from a std::string_view.
     * @param s A string_view representing the address.
     */
    bluetoothAddress(std::string_view s) : raw_(s) {}

    /**
     * @brief Returns the raw address string.
     * @return A string_view of the raw address.
     */
    std::string_view raw() const { return raw_; }

    /**
     * @brief Returns the canonical (uppercase) address string.
     * @return A string containing the uppercase address.
     */
    std::string to_string() const { return to_upper_ascii(raw_); }

    /**
     * @brief Conversion operator to convert bluetoothAddress to std::string.
     *
     * @note This is provided to return the canonical (uppercase) address.
     * @return The canonical (uppercase) address string.
     */
    operator std::string() const { return to_string(); }

    /**
     * @brief Equality operator for case-insensitive comparison.
     * @param a The first bluetoothAddress.
     * @param b The second bluetoothAddress.
     * @return True if addresses are equal (case-insensitive), false otherwise.
     */
    friend bool operator==(const bluetoothAddress& a, const bluetoothAddress& b) {
        return compare_ci_ascii(a.raw_, b.raw_) == 0;
    }

    /**
     * @brief Less-than operator for case-insensitive comparison.
     * @param a The first bluetoothAddress.
     * @param b The second bluetoothAddress.
     * @return True if a is less than b (case-insensitive), false otherwise.
     */
    friend bool operator<(const bluetoothAddress& a, const bluetoothAddress& b) {
        return compare_ci_ascii(a.raw_, b.raw_) < 0;
    }

  private:
    std::string raw_;

    //! @cond Doxygen_Suppress
    static char upper_ascii(char c) {
        if (c >= 'a' && c <= 'z') return static_cast<char>(c - ('a' - 'A'));
        return c;
    }

    static int compare_ci_ascii(std::string_view a, std::string_view b) {
        const size_t n = (a.size() < b.size()) ? a.size() : b.size();
        for (size_t i = 0; i < n; ++i) {
            const char ca = upper_ascii(a[i]);
            const char cb = upper_ascii(b[i]);
            if (ca < cb) return -1;
            if (ca > cb) return 1;
        }
        if (a.size() < b.size()) return -1;
        if (a.size() > b.size()) return 1;
        return 0;
    }

    static std::string to_upper_ascii(std::string_view s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) out.push_back(upper_ascii(c));
        return out;
    }
    //! @endcond
};

using BluetoothAddress = bluetoothAddress;

// IDEA: Extend BluetoothUUID to include a `uuid` function that
// returns the same string, but provides a homogeneous interface.
using BluetoothUUID = std::string;

/**
 * @typedef ByteArray
 * @brief Represents a byte array using kvn::bytearray from the external library.
 */
using ByteArray = kvn::bytearray;

#ifdef ANDROID
#pragma push_macro("ANDROID")
#undef ANDROID
#define ANDROID_WAS_DEFINED
#endif

enum class OperatingSystem {
    WINDOWS,
    MACOS,
    IOS,
    LINUX,
    ANDROID,
};

#ifdef ANDROID_WAS_DEFINED
#pragma pop_macro("ANDROID")
#undef ANDROID_WAS_DEFINED
#endif

// TODO: Add to_string functions for all enums.
enum BluetoothAddressType : int32_t { PUBLIC = 0, RANDOM = 1, UNSPECIFIED = 2 };

}  // namespace SimpleBLE
