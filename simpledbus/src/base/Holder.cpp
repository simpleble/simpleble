#include <simpledbus/base/Holder.h>
#include <iomanip>
#include <sstream>

#include "dbus/dbus-protocol.h"

using namespace SimpleDBus;

Holder::Holder() {}

Holder::~Holder() {}

// Template specializations for create
template <>
Holder Holder::create(bool value) {
    Holder h;
    h._type = BOOLEAN;
    h.holder_boolean = value;
    return h;
}

template <>
Holder Holder::create(uint8_t value) {
    Holder h;
    h._type = BYTE;
    h.holder_integer = value;
    return h;
}

template <>
Holder Holder::create(int16_t value) {
    Holder h;
    h._type = INT16;
    h.holder_integer = value;
    return h;
}

template <>
Holder Holder::create(uint16_t value) {
    Holder h;
    h._type = UINT16;
    h.holder_integer = value;
    return h;
}

template <>
Holder Holder::create(int32_t value) {
    Holder h;
    h._type = INT32;
    h.holder_integer = value;
    return h;
}

template <>
Holder Holder::create(uint32_t value) {
    Holder h;
    h._type = UINT32;
    h.holder_integer = value;
    return h;
}

template <>
Holder Holder::create(int64_t value) {
    Holder h;
    h._type = INT64;
    h.holder_integer = value;
    return h;
}

template <>
Holder Holder::create(uint64_t value) {
    Holder h;
    h._type = UINT64;
    h.holder_integer = value;
    return h;
}

template <>
Holder Holder::create(double value) {
    Holder h;
    h._type = DOUBLE;
    h.holder_double = value;
    return h;
}

template <>
Holder Holder::create(std::string value) {
    Holder h;
    h._type = STRING;
    h.holder_string = value;
    return h;
}

template <>
Holder Holder::create(ObjectPath value) {
    Holder h;
    h._type = OBJ_PATH;
    h.holder_string = static_cast<std::string>(value);
    return h;
}

template <>
Holder Holder::create(Signature value) {
    Holder h;
    h._type = SIGNATURE;
    h.holder_string = static_cast<std::string>(value);
    return h;
}

template <>
Holder Holder::create<std::vector<Holder>>() {
    Holder h;
    h._type = ARRAY;
    h.holder_array.clear();
    return h;
}

template <>
Holder Holder::create<std::map<std::string, Holder>>() {
    Holder h;
    h._type = DICT;
    h.holder_dict.clear();
    return h;
}

// Named creation functions (deprecated)
Holder Holder::create_byte(uint8_t value) { return create<uint8_t>(value); }
Holder Holder::create_boolean(bool value) { return create<bool>(value); }
Holder Holder::create_int16(int16_t value) { return create<int16_t>(value); }
Holder Holder::create_uint16(uint16_t value) { return create<uint16_t>(value); }
Holder Holder::create_int32(int32_t value) { return create<int32_t>(value); }
Holder Holder::create_uint32(uint32_t value) { return create<uint32_t>(value); }
Holder Holder::create_int64(int64_t value) { return create<int64_t>(value); }
Holder Holder::create_uint64(uint64_t value) { return create<uint64_t>(value); }
Holder Holder::create_double(double value) { return create<double>(value); }
Holder Holder::create_string(const std::string& str) { return create<std::string>(str); }
Holder Holder::create_object_path(const ObjectPath& path) { return create<ObjectPath>(path); }
Holder Holder::create_signature(const Signature& signature) { return create<Signature>(signature); }
Holder Holder::create_array() { return create<std::vector<Holder>>(); }
Holder Holder::create_dict() { return create<std::map<std::string, Holder>>(); }

// Template specializations for get
template <>
bool Holder::get() const {
    return holder_boolean;
}

template <>
uint8_t Holder::get() const {
    return (uint8_t)(holder_integer & 0x00000000000000FFL);
}

template <>
int16_t Holder::get() const {
    return (int16_t)(holder_integer & 0x000000000000FFFFL);
}

template <>
uint16_t Holder::get() const {
    return (uint16_t)(holder_integer & 0x000000000000FFFFL);
}

template <>
int32_t Holder::get() const {
    return (int32_t)(holder_integer & 0x00000000FFFFFFFFL);
}

template <>
uint32_t Holder::get() const {
    return (uint32_t)(holder_integer & 0x00000000FFFFFFFFL);
}

template <>
int64_t Holder::get() const {
    return (int64_t)holder_integer;
}

template <>
uint64_t Holder::get() const {
    return holder_integer;
}

template <>
double Holder::get() const {
    return holder_double;
}

template <>
std::string Holder::get() const {
    return holder_string;
}

template <>
ObjectPath Holder::get() const {
    return ObjectPath(holder_string);
}

template <>
Signature Holder::get() const {
    return Signature(holder_string);
}

template <>
std::vector<Holder> Holder::get() const {
    return holder_array;
}

template <>
std::map<uint8_t, Holder> Holder::get() const {
    return _get_dict<uint8_t>(BYTE);
}

template <>
std::map<uint16_t, Holder> Holder::get() const {
    return _get_dict<uint16_t>(UINT16);
}

template <>
std::map<uint32_t, Holder> Holder::get() const {
    return _get_dict<uint32_t>(UINT32);
}

template <>
std::map<uint64_t, Holder> Holder::get() const {
    return _get_dict<uint64_t>(UINT64);
}

template <>
std::map<int16_t, Holder> Holder::get() const {
    return _get_dict<int16_t>(INT16);
}

template <>
std::map<int32_t, Holder> Holder::get() const {
    return _get_dict<int32_t>(INT32);
}

template <>
std::map<int64_t, Holder> Holder::get() const {
    return _get_dict<int64_t>(INT64);
}

template <>
std::map<std::string, Holder> Holder::get() const {
    return _get_dict<std::string>(STRING);
}

template <>
std::map<ObjectPath, Holder> Holder::get() const {
    std::map<ObjectPath, Holder> output;
    for (auto& [key_type_internal, key, value] : holder_dict) {
        if (key_type_internal == OBJ_PATH) {
            output[ObjectPath(std::any_cast<std::string>(key))] = value;
        }
    }
    return output;
}

template <>
std::map<Signature, Holder> Holder::get() const {
    std::map<Signature, Holder> output;
    for (auto& [key_type_internal, key, value] : holder_dict) {
        if (key_type_internal == SIGNATURE) {
            output[Signature(std::any_cast<std::string>(key))] = value;
        }
    }
    return output;
}

bool Holder::operator!=(const Holder& other) const { return !(*this == other); }

bool Holder::operator==(const Holder& other) const {
    if (type() != other.type()) {
        return false;
    }

    switch (type()) {
        case NONE:
            return true;
        case BYTE:
            return get<uint8_t>() == other.get<uint8_t>();
        case BOOLEAN:
            return get<bool>() == other.get<bool>();
        case INT16:
            return get<int16_t>() == other.get<int16_t>();
        case UINT16:
            return get<uint16_t>() == other.get<uint16_t>();
        case INT32:
            return get<int32_t>() == other.get<int32_t>();
        case UINT32:
            return get<uint32_t>() == other.get<uint32_t>();
        case INT64:
            return get<int64_t>() == other.get<int64_t>();
        case UINT64:
            return get<uint64_t>() == other.get<uint64_t>();
        case DOUBLE:
            return get<double>() == other.get<double>();
        case STRING:
            return get<std::string>() == other.get<std::string>();
        case OBJ_PATH:
            return get<ObjectPath>() == other.get<ObjectPath>();
        case SIGNATURE:
            return get<Signature>() == other.get<Signature>();
        case ARRAY:
            return get<std::vector<Holder>>() == other.get<std::vector<Holder>>();
        case DICT:
            return (get<std::map<uint8_t, Holder>>() == other.get<std::map<uint8_t, Holder>>()) &&
                   (get<std::map<uint16_t, Holder>>() == other.get<std::map<uint16_t, Holder>>()) &&
                   (get<std::map<int16_t, Holder>>() == other.get<std::map<int16_t, Holder>>()) &&
                   (get<std::map<uint32_t, Holder>>() == other.get<std::map<uint32_t, Holder>>()) &&
                   (get<std::map<int32_t, Holder>>() == other.get<std::map<int32_t, Holder>>()) &&
                   (get<std::map<uint64_t, Holder>>() == other.get<std::map<uint64_t, Holder>>()) &&
                   (get<std::map<int64_t, Holder>>() == other.get<std::map<int64_t, Holder>>()) &&
                   (get<std::map<std::string, Holder>>() == other.get<std::map<std::string, Holder>>()) &&
                   (get<std::map<ObjectPath, Holder>>() == other.get<std::map<ObjectPath, Holder>>()) &&
                   (get<std::map<Signature, Holder>>() == other.get<std::map<Signature, Holder>>());
        default:
            return false;
    }
}

Holder::Type Holder::type() const { return _type; }

// Named getter functions (deprecated)
bool Holder::get_boolean() const { return get<bool>(); }
uint8_t Holder::get_byte() const { return get<uint8_t>(); }
int16_t Holder::get_int16() const { return get<int16_t>(); }
uint16_t Holder::get_uint16() const { return get<uint16_t>(); }
int32_t Holder::get_int32() const { return get<int32_t>(); }
uint32_t Holder::get_uint32() const { return get<uint32_t>(); }
int64_t Holder::get_int64() const { return get<int64_t>(); }
uint64_t Holder::get_uint64() const { return get<uint64_t>(); }
double Holder::get_double() const { return get<double>(); }
std::string Holder::get_string() const { return get<std::string>(); }
ObjectPath Holder::get_object_path() const { return get<ObjectPath>(); }
Signature Holder::get_signature() const { return get<Signature>(); }
std::vector<Holder> Holder::get_array() const { return get<std::vector<Holder>>(); }
std::map<uint8_t, Holder> Holder::get_dict_uint8() const { return get<std::map<uint8_t, Holder>>(); }
std::map<uint16_t, Holder> Holder::get_dict_uint16() const { return get<std::map<uint16_t, Holder>>(); }
std::map<uint32_t, Holder> Holder::get_dict_uint32() const { return get<std::map<uint32_t, Holder>>(); }
std::map<uint64_t, Holder> Holder::get_dict_uint64() const { return get<std::map<uint64_t, Holder>>(); }
std::map<int16_t, Holder> Holder::get_dict_int16() const { return get<std::map<int16_t, Holder>>(); }
std::map<int32_t, Holder> Holder::get_dict_int32() const { return get<std::map<int32_t, Holder>>(); }
std::map<int64_t, Holder> Holder::get_dict_int64() const { return get<std::map<int64_t, Holder>>(); }
std::map<std::string, Holder> Holder::get_dict_string() const { return get<std::map<std::string, Holder>>(); }
std::map<ObjectPath, Holder> Holder::get_dict_object_path() const { return get<std::map<ObjectPath, Holder>>(); }
std::map<Signature, Holder> Holder::get_dict_signature() const { return get<std::map<Signature, Holder>>(); }

std::string Holder::_represent_simple() const {
    std::ostringstream output;
    output << std::boolalpha;

    switch (_type) {
        case BOOLEAN:
            output << get<bool>();
            break;
        case BYTE:
            output << (int)get<uint8_t>();
            break;
        case INT16:
            output << (int)get<int16_t>();
            break;
        case UINT16:
            output << (int)get<uint16_t>();
            break;
        case INT32:
            output << get<int32_t>();
            break;
        case UINT32:
            output << get<uint32_t>();
            break;
        case INT64:
            output << get<int64_t>();
            break;
        case UINT64:
            output << get<uint64_t>();
            break;
        case DOUBLE:
            output << get<double>();
            break;
        case STRING:
        case OBJ_PATH:
        case SIGNATURE:
            output << get<std::string>();
            break;
        default:
            break;
    }
    return output.str();
}

std::vector<std::string> Holder::_represent_container() const {
    std::vector<std::string> output_lines;
    switch (_type) {
        case BOOLEAN:
        case BYTE:
        case INT16:
        case UINT16:
        case INT32:
        case UINT32:
        case INT64:
        case UINT64:
        case DOUBLE:
        case STRING:
        case OBJ_PATH:
        case SIGNATURE:
            output_lines.push_back(_represent_simple());
            break;
        case ARRAY: {
            output_lines.push_back("Array:");
            std::vector<std::string> additional_lines;
            if (holder_array.size() > 0 && holder_array[0]._type == BYTE) {
                // Dealing with an array of bytes, use custom print functionality.
                std::string temp_line = "";
                for (size_t i = 0; i < holder_array.size(); i++) {
                    // Represent each byte as a hex string
                    std::stringstream stream;
                    stream << std::setfill('0') << std::setw(2) << std::hex << ((int)holder_array[i].get<uint8_t>());
                    temp_line += (stream.str() + " ");
                    if ((i + 1) % 32 == 0) {
                        additional_lines.push_back(temp_line);
                        temp_line = "";
                    }
                }
                additional_lines.push_back(temp_line);
            } else {
                for (size_t i = 0; i < holder_array.size(); i++) {
                    for (auto& line : holder_array[i]._represent_container()) {
                        additional_lines.push_back(line);
                    }
                }
            }
            for (auto& line : additional_lines) {
                output_lines.push_back("  " + line);
            }
            break;
        }
        case DICT:
            output_lines.push_back("Dictionary:");
            for (auto& [key_type_internal, key, value] : holder_dict) {
                output_lines.push_back(_represent_type(key_type_internal, key) + ":");
                auto additional_lines = value._represent_container();
                for (auto& line : additional_lines) {
                    output_lines.push_back("  " + line);
                }
            }
            break;
        default:
            break;
    }
    return output_lines;
}

std::string Holder::represent() const {
    std::ostringstream output;
    auto output_lines = _represent_container();
    for (auto& output_line : output_lines) {
        output << output_line << std::endl;
    }
    return output.str();
}

std::string Holder::_signature_simple() const {
    switch (_type) {
        case BOOLEAN:
            return DBUS_TYPE_BOOLEAN_AS_STRING;
        case BYTE:
            return DBUS_TYPE_BYTE_AS_STRING;
        case INT16:
            return DBUS_TYPE_INT16_AS_STRING;
        case UINT16:
            return DBUS_TYPE_UINT16_AS_STRING;
        case INT32:
            return DBUS_TYPE_INT32_AS_STRING;
        case UINT32:
            return DBUS_TYPE_UINT32_AS_STRING;
        case INT64:
            return DBUS_TYPE_INT64_AS_STRING;
        case UINT64:
            return DBUS_TYPE_UINT64_AS_STRING;
        case DOUBLE:
            return DBUS_TYPE_DOUBLE_AS_STRING;
        case STRING:
            return DBUS_TYPE_STRING_AS_STRING;
        case OBJ_PATH:
            return DBUS_TYPE_OBJECT_PATH_AS_STRING;
        case SIGNATURE:
            return DBUS_TYPE_SIGNATURE_AS_STRING;
        default:
            return "";
    }
}

std::string Holder::_signature_type(Type type) noexcept {
    switch (type) {
        case BOOLEAN:
            return DBUS_TYPE_BOOLEAN_AS_STRING;
        case BYTE:
            return DBUS_TYPE_BYTE_AS_STRING;
        case INT16:
            return DBUS_TYPE_INT16_AS_STRING;
        case UINT16:
            return DBUS_TYPE_UINT16_AS_STRING;
        case INT32:
            return DBUS_TYPE_INT32_AS_STRING;
        case UINT32:
            return DBUS_TYPE_UINT32_AS_STRING;
        case INT64:
            return DBUS_TYPE_INT64_AS_STRING;
        case UINT64:
            return DBUS_TYPE_UINT64_AS_STRING;
        case DOUBLE:
            return DBUS_TYPE_DOUBLE_AS_STRING;
        case STRING:
            return DBUS_TYPE_STRING_AS_STRING;
        case OBJ_PATH:
            return DBUS_TYPE_OBJECT_PATH_AS_STRING;
        case SIGNATURE:
            return DBUS_TYPE_SIGNATURE_AS_STRING;
        default:
            return "";
    }
}

std::string Holder::_represent_type(Type type, std::any value) noexcept {
    std::ostringstream output;
    output << std::boolalpha;

    switch (type) {
        case BOOLEAN:
            output << std::any_cast<bool>(value);
            break;
        case BYTE:
            output << std::any_cast<uint8_t>(value);
            break;
        case INT16:
            output << std::any_cast<int16_t>(value);
            break;
        case UINT16:
            output << std::any_cast<uint16_t>(value);
            break;
        case INT32:
            output << std::any_cast<int32_t>(value);
            break;
        case UINT32:
            output << std::any_cast<uint32_t>(value);
            break;
        case INT64:
            output << std::any_cast<int64_t>(value);
            break;
        case UINT64:
            output << std::any_cast<uint64_t>(value);
            break;
        case DOUBLE:
            output << std::any_cast<double>(value);
            break;
        case STRING:
        case OBJ_PATH:
        case SIGNATURE:
            output << std::any_cast<std::string>(value);
            break;
        default:
            break;
    }
    return output.str();
}

void Holder::signature_override(const std::string& signature) {
    // TODO: Check that the signature is valid for the Holder type and contents.
    _signature = signature;
}

std::string Holder::signature() const {
    if (_signature) {
        return *_signature;
    }

    std::string output;
    switch (_type) {
        case BOOLEAN:
        case BYTE:
        case INT16:
        case UINT16:
        case INT32:
        case UINT32:
        case INT64:
        case UINT64:
        case DOUBLE:
        case STRING:
        case OBJ_PATH:
        case SIGNATURE:
            output = _signature_simple();
            break;
        case ARRAY:
            output = DBUS_TYPE_ARRAY_AS_STRING;
            if (holder_array.size() == 0) {
                output += DBUS_TYPE_VARIANT_AS_STRING;
            } else {
                // Check if all elements of holder_array are the same type
                auto first_type = holder_array[0]._type;
                bool all_same_type = true;
                for (auto& element : holder_array) {
                    if (element._type != first_type) {
                        all_same_type = false;
                        break;
                    }
                }

                if (all_same_type) {
                    output += holder_array[0]._signature_simple();
                } else {
                    output += DBUS_TYPE_VARIANT_AS_STRING;
                }
            }
            break;
        case DICT:
            output = DBUS_TYPE_ARRAY_AS_STRING;
            output += DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING;

            if (holder_dict.size() == 0) {
                output += DBUS_TYPE_STRING_AS_STRING;
                output += DBUS_TYPE_VARIANT_AS_STRING;
            } else {
                // Check if all keys of holder_dict are the same type
                auto first_key_type = std::get<0>(holder_dict[0]);
                bool all_same_key_type = true;
                for (auto& [key_type_internal, key, value] : holder_dict) {
                    if (key_type_internal != first_key_type) {
                        all_same_key_type = false;
                        break;
                    }
                }
                if (all_same_key_type) {
                    output += _signature_type(first_key_type);
                } else {
                    output += DBUS_TYPE_VARIANT_AS_STRING;
                }

                // Check if all values of holder_dict are the same type
                auto first_value_type = std::get<2>(holder_dict[0])._type;
                bool all_same_value_type = true;
                for (auto& [key_type_internal, key, value] : holder_dict) {
                    if (value._type != first_value_type) {
                        all_same_value_type = false;
                        break;
                    }
                }

                if (all_same_value_type && first_value_type != ARRAY && first_value_type != DICT) {
                    output += std::get<2>(holder_dict[0])._signature_simple();
                } else {
                    output += DBUS_TYPE_VARIANT_AS_STRING;
                }
            }

            output += DBUS_DICT_ENTRY_END_CHAR_AS_STRING;
            break;
        default:
            break;
    }
    return output;
}


std::any Holder::get_contents() const {
    // Only return the contents for simple types
    switch (_type) {
        case BOOLEAN:
            return get<bool>();
        case BYTE:
            return get<uint8_t>();
        case INT16:
            return get<int16_t>();
        case UINT16:
            return get<uint16_t>();
        case INT32:
            return get<int32_t>();
        case UINT32:
            return get<uint32_t>();
        case INT64:
            return get<int64_t>();
        case UINT64:
            return get<uint64_t>();
        case DOUBLE:
            return get<double>();
        case STRING:
            return get<std::string>();
        case OBJ_PATH:
            return (std::string)get<ObjectPath>();
        case SIGNATURE:
            return (std::string)get<Signature>();
        default:
            return std::any();
    }
}

void Holder::array_append(Holder holder) { holder_array.push_back(holder); }

void Holder::dict_append(Type key_type, std::any key, Holder value) {
    if (key.type() == typeid(const char*)) {
        key = std::string(std::any_cast<const char*>(key));
    }

    // TODO : VALIDATE THAT THE SPECIFIED KEY TYPE IS CORRECT

    holder_dict.push_back(std::make_tuple(key_type, key, value));
}

template <typename T>
std::map<T, Holder> Holder::_get_dict(Type key_type) const {
    std::map<T, Holder> output;
    for (auto& [key_type_internal, key, value] : holder_dict) {
        if (key_type_internal == key_type) {
            output[std::any_cast<T>(key)] = value;
        }
    }
    return output;
}
