#pragma once

#include <simpledbus/base/Holder.h>
#include <cstdint>
#include <vector>

namespace SimpleDBus {

namespace HolderUtils {

Holder from_byte_array(const std::vector<uint8_t>& value);
std::vector<uint8_t> to_byte_array(const Holder& holder);

Holder from_string_array(const std::vector<std::string>& value);
std::vector<std::string> to_string_array(const Holder& holder);

Holder from_dict_uint16_byte_array(const std::map<uint16_t, std::vector<uint8_t>>& value);
std::map<uint16_t, std::vector<uint8_t>> to_dict_uint16_byte_array(const Holder& holder);

Holder from_dict_string_byte_array(const std::map<std::string, std::vector<uint8_t>>& value);
std::map<std::string, std::vector<uint8_t>> to_dict_string_byte_array(const Holder& holder);

}  // namespace HolderUtils

}  // namespace SimpleDBus
