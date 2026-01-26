#include <simpledbus/base/HolderUtils.h>

namespace SimpleDBus {

namespace HolderUtils {

Holder from_byte_array(const std::vector<uint8_t>& value) { return Holder::create<std::vector<uint8_t>>(value); }

std::vector<uint8_t> to_byte_array(const Holder& holder) { return holder.get<std::vector<uint8_t>>(); }

Holder from_string_array(const std::vector<std::string>& value) {
    return Holder::create<std::vector<std::string>>(value);
}

std::vector<std::string> to_string_array(const Holder& holder) { return holder.get<std::vector<std::string>>(); }

Holder from_dict_uint8_byte_array(const std::map<uint8_t, std::vector<uint8_t>>& value) {
    return Holder::create<std::map<uint8_t, std::vector<uint8_t>>>(value);
}

std::map<uint8_t, std::vector<uint8_t>> to_dict_uint8_byte_array(const Holder& holder) {
    return holder.get<std::map<uint8_t, std::vector<uint8_t>>>();
}

Holder from_dict_uint16_byte_array(const std::map<uint16_t, std::vector<uint8_t>>& value) {
    return Holder::create<std::map<uint16_t, std::vector<uint8_t>>>(value);
}

std::map<uint16_t, std::vector<uint8_t>> to_dict_uint16_byte_array(const Holder& holder) {
    return holder.get<std::map<uint16_t, std::vector<uint8_t>>>();
}

Holder from_dict_string_byte_array(const std::map<std::string, std::vector<uint8_t>>& value) {
    return Holder::create<std::map<std::string, std::vector<uint8_t>>>(value);
}

std::map<std::string, std::vector<uint8_t>> to_dict_string_byte_array(const Holder& holder) {
    return holder.get<std::map<std::string, std::vector<uint8_t>>>();
}

}  // namespace HolderUtils

}  // namespace SimpleDBus
