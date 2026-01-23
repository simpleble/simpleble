#pragma once

#include <kvn/kvn_bytearray.h>
#include <simpledbus/base/Holder.h>
#include <cstdint>
#include <map>
#include <vector>

namespace SimpleDBus {

namespace HolderUtils {

Holder from_byte_array(const kvn::bytearray& value);
kvn::bytearray to_byte_array(const Holder& holder);

Holder from_string_array(const std::vector<std::string>& value);
std::vector<std::string> to_string_array(const Holder& holder);

Holder from_dict_uint8_byte_array(const std::map<uint8_t, kvn::bytearray>& value);
std::map<uint8_t, kvn::bytearray> to_dict_uint8_byte_array(const Holder& holder);

Holder from_dict_uint16_byte_array(const std::map<uint16_t, kvn::bytearray>& value);
std::map<uint16_t, kvn::bytearray> to_dict_uint16_byte_array(const Holder& holder);

Holder from_dict_string_byte_array(const std::map<std::string, kvn::bytearray>& value);
std::map<std::string, kvn::bytearray> to_dict_string_byte_array(const Holder& holder);

}  // namespace HolderUtils

}  // namespace SimpleDBus
