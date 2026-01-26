#include <simpledbus/base/HolderUtils.h>

namespace SimpleDBus {

namespace HolderUtils {

Holder from_byte_array(const kvn::bytearray& value) {
    Holder value_array = Holder::create<std::vector<Holder>>();
    for (size_t i = 0; i < value.size(); i++) {
        value_array.array_append(Holder::create<uint8_t>(value[i]));
    }
    return value_array;
}

kvn::bytearray to_byte_array(const Holder& holder) {
    auto value_array = holder.get<std::vector<Holder>>();
    kvn::bytearray _value;
    for (std::size_t i = 0; i < value_array.size(); i++) {
        _value.push_back(value_array[i].get<uint8_t>());
    }
    return _value;
}

Holder from_string_array(const std::vector<std::string>& value) {
    Holder holder = Holder::create<std::vector<Holder>>();
    for (auto const& val : value) {
        holder.array_append(Holder::create<std::string>(val));
    }
    return holder;
}

std::vector<std::string> to_string_array(const Holder& holder) {
    std::vector<std::string> result;
    auto array = holder.get<std::vector<Holder>>();
    for (auto& h : array) {
        result.push_back(h.get<std::string>());
    }
    return result;
}

Holder from_dict_uint8_byte_array(const std::map<uint8_t, kvn::bytearray>& value) {
    Holder dict = Holder::create<std::map<std::string, Holder>>();
    for (auto const& [key, val] : value) {
        dict.dict_append(Holder::BYTE, key, from_byte_array(val));
    }
    return dict;
}

std::map<uint8_t, kvn::bytearray> to_dict_uint8_byte_array(const Holder& holder) {
    std::map<uint8_t, kvn::bytearray> result;
    auto dict = holder.get<std::map<uint8_t, Holder>>();
    for (auto& [key, val_holder] : dict) {
        result[key] = to_byte_array(val_holder);
    }
    return result;
}

Holder from_dict_uint16_byte_array(const std::map<uint16_t, kvn::bytearray>& value) {
    Holder dict = Holder::create<std::map<std::string, Holder>>();
    for (auto const& [key, val] : value) {
        dict.dict_append(Holder::UINT16, key, from_byte_array(val));
    }
    return dict;
}

std::map<uint16_t, kvn::bytearray> to_dict_uint16_byte_array(const Holder& holder) {
    std::map<uint16_t, kvn::bytearray> result;
    auto dict = holder.get<std::map<uint16_t, Holder>>();
    for (auto& [key, val_holder] : dict) {
        result[key] = to_byte_array(val_holder);
    }
    return result;
}

Holder from_dict_string_byte_array(const std::map<std::string, kvn::bytearray>& value) {
    Holder dict = Holder::create<std::map<std::string, Holder>>();
    for (auto const& [key, val] : value) {
        dict.dict_append(Holder::STRING, key, from_byte_array(val));
    }
    return dict;
}

std::map<std::string, kvn::bytearray> to_dict_string_byte_array(const Holder& holder) {
    std::map<std::string, kvn::bytearray> result;
    auto dict = holder.get<std::map<std::string, Holder>>();
    for (auto& [key, val_holder] : dict) {
        result[key] = to_byte_array(val_holder);
    }
    return result;
}

}  // namespace HolderUtils

}  // namespace SimpleDBus
