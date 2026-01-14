#include <simpledbus/base/HolderUtils.h>

namespace SimpleDBus {

namespace HolderUtils {

Holder from_byte_array(const std::vector<uint8_t>& value) {
    Holder value_array = Holder::create_array();
    for (size_t i = 0; i < value.size(); i++) {
        value_array.array_append(Holder::create_byte(value[i]));
    }
    return value_array;
}

std::vector<uint8_t> to_byte_array(const Holder& holder) {
    auto value_array = holder.get_array();
    std::vector<uint8_t> _value;
    for (std::size_t i = 0; i < value_array.size(); i++) {
        _value.push_back(value_array[i].get_byte());
    }
    return _value;
}

Holder from_string_array(const std::vector<std::string>& value) {
    Holder holder = Holder::create_array();
    for (auto const& val : value) {
        holder.array_append(Holder::create_string(val));
    }
    return holder;
}

std::vector<std::string> to_string_array(const Holder& holder) {
    std::vector<std::string> result;
    auto array = holder.get_array();
    for (auto& h : array) {
        result.push_back(h.get_string());
    }
    return result;
}

Holder from_dict_uint16_byte_array(const std::map<uint16_t, std::vector<uint8_t>>& value) {
    Holder dict = Holder::create_dict();
    for (auto const& [key, val] : value) {
        dict.dict_append(Holder::UINT16, key, from_byte_array(val));
    }
    return dict;
}

std::map<uint16_t, std::vector<uint8_t>> to_dict_uint16_byte_array(const Holder& holder) {
    std::map<uint16_t, std::vector<uint8_t>> result;
    auto dict = holder.get_dict_uint16();
    for (auto& [key, val_holder] : dict) {
        result[key] = to_byte_array(val_holder);
    }
    return result;
}

Holder from_dict_string_byte_array(const std::map<std::string, std::vector<uint8_t>>& value) {
    Holder dict = Holder::create_dict();
    for (auto const& [key, val] : value) {
        dict.dict_append(Holder::STRING, key, from_byte_array(val));
    }
    return dict;
}

std::map<std::string, std::vector<uint8_t>> to_dict_string_byte_array(const Holder& holder) {
    std::map<std::string, std::vector<uint8_t>> result;
    auto dict = holder.get_dict_string();
    for (auto& [key, val_holder] : dict) {
        result[key] = to_byte_array(val_holder);
    }
    return result;
}

}  // namespace HolderUtils

}  // namespace SimpleDBus
