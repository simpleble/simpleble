#pragma once

#include <any>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace SimpleDBus {

class ObjectPath {
  public:
    ObjectPath() = default;
    ObjectPath(const std::string& path) : path(path) {}
    ObjectPath(const char* path) : path(path) {}
    operator std::string() const { return path; }
    const char* c_str() const { return path.c_str(); }
    bool operator<(const ObjectPath& other) const { return path < other.path; }
    bool operator==(const ObjectPath& other) const { return path == other.path; }
    bool operator!=(const ObjectPath& other) const { return path != other.path; }

  private:
    std::string path;
};

class Signature {
  public:
    Signature() = default;
    Signature(const std::string& signature) : signature(signature) {}
    Signature(const char* signature) : signature(signature) {}
    operator std::string() const { return signature; }
    const char* c_str() const { return signature.c_str(); }
    bool operator<(const Signature& other) const { return signature < other.signature; }
    bool operator==(const Signature& other) const { return signature == other.signature; }
    bool operator!=(const Signature& other) const { return signature != other.signature; }

  private:
    std::string signature;
};

class Holder;

class Holder {
  public:
    Holder();
    ~Holder();

    bool operator!=(const Holder& rhs) const;
    bool operator==(const Holder& rhs) const;

    typedef enum {
        NONE,
        BYTE,
        BOOLEAN,
        INT16,
        UINT16,
        INT32,
        UINT32,
        INT64,
        UINT64,
        DOUBLE,
        STRING,
        OBJ_PATH,
        SIGNATURE,
        ARRAY,
        DICT
    } Type;

    Type type() const;
    std::string represent() const;
    std::string signature() const;
    void signature_override(const std::string& signature);

    [[deprecated("Use create<bool> instead")]] static Holder create_boolean(bool value);
    [[deprecated("Use create<uint8_t> instead")]] static Holder create_byte(uint8_t value);
    [[deprecated("Use create<int16_t> instead")]] static Holder create_int16(int16_t value);
    [[deprecated("Use create<uint16_t> instead")]] static Holder create_uint16(uint16_t value);
    [[deprecated("Use create<int32_t> instead")]] static Holder create_int32(int32_t value);
    [[deprecated("Use create<uint32_t> instead")]] static Holder create_uint32(uint32_t value);
    [[deprecated("Use create<int64_t> instead")]] static Holder create_int64(int64_t value);
    [[deprecated("Use create<uint64_t> instead")]] static Holder create_uint64(uint64_t value);
    [[deprecated("Use create<double> instead")]] static Holder create_double(double value);
    [[deprecated("Use create<std::string> instead")]] static Holder create_string(const std::string& str);
    [[deprecated("Use create<ObjectPath> instead")]] static Holder create_object_path(const ObjectPath& path);
    [[deprecated("Use create<Signature> instead")]] static Holder create_signature(const Signature& signature);
    [[deprecated("Use create<std::vector<Holder>> instead")]] static Holder create_array();
    [[deprecated("Use create<std::map<std::string, Holder>> instead")]] static Holder create_dict();

    std::any get_contents() const;

    [[deprecated("Use get<bool> instead")]] bool get_boolean() const;
    [[deprecated("Use get<uint8_t> instead")]] uint8_t get_byte() const;
    [[deprecated("Use get<int16_t> instead")]] int16_t get_int16() const;
    [[deprecated("Use get<uint16_t> instead")]] uint16_t get_uint16() const;
    [[deprecated("Use get<int32_t> instead")]] int32_t get_int32() const;
    [[deprecated("Use get<uint32_t> instead")]] uint32_t get_uint32() const;
    [[deprecated("Use get<int64_t> instead")]] int64_t get_int64() const;
    [[deprecated("Use get<uint64_t> instead")]] uint64_t get_uint64() const;
    [[deprecated("Use get<double> instead")]] double get_double() const;
    [[deprecated("Use get<std::string> instead")]] std::string get_string() const;
    [[deprecated("Use get<ObjectPath> instead")]] ObjectPath get_object_path() const;
    [[deprecated("Use get<Signature> instead")]] Signature get_signature() const;
    [[deprecated("Use get<std::vector<Holder>> instead")]] std::vector<Holder> get_array() const;
    [[deprecated("Use get<std::map<uint8_t, Holder>> instead")]] std::map<uint8_t, Holder> get_dict_uint8() const;
    [[deprecated("Use get<std::map<uint16_t, Holder>> instead")]] std::map<uint16_t, Holder> get_dict_uint16() const;
    [[deprecated("Use get<std::map<uint32_t, Holder>> instead")]] std::map<uint32_t, Holder> get_dict_uint32() const;
    [[deprecated("Use get<std::map<uint64_t, Holder>> instead")]] std::map<uint64_t, Holder> get_dict_uint64() const;
    [[deprecated("Use get<std::map<int16_t, Holder>> instead")]] std::map<int16_t, Holder> get_dict_int16() const;
    [[deprecated("Use get<std::map<int32_t, Holder>> instead")]] std::map<int32_t, Holder> get_dict_int32() const;
    [[deprecated("Use get<std::map<int64_t, Holder>> instead")]] std::map<int64_t, Holder> get_dict_int64() const;
    [[deprecated("Use get<std::map<std::string, Holder>> instead")]] std::map<std::string, Holder> get_dict_string() const;
    [[deprecated("Use get<std::map<ObjectPath, Holder>> instead")]] std::map<ObjectPath, Holder> get_dict_object_path() const;
    [[deprecated("Use get<std::map<Signature, Holder>> instead")]] std::map<Signature, Holder> get_dict_signature() const;

    void dict_append(Type key_type, std::any key, Holder value);
    void array_append(Holder holder);

    // Template speciallizations.
    template <typename T>
    static Holder create();

    template <typename T>
    static Holder create(T value);

    template <typename T>
    T get() const;

  private:
    Type _type = NONE;
    std::optional<std::string> _signature;

    bool holder_boolean = false;
    uint64_t holder_integer = 0;
    double holder_double = 0;
    std::string holder_string;
    std::vector<Holder> holder_array;

    // Dictionaries are stored within a vector as a tuple of <key_type, key, holder>
    std::vector<std::tuple<Type, std::any, Holder>> holder_dict;

    std::vector<std::string> _represent_container() const;
    std::string _represent_simple() const;
    std::string _signature_simple() const;

    template <typename T>
    std::map<T, Holder> _get_dict(Type key_type) const;

    static std::string _signature_type(Type type) noexcept;
    static std::string _represent_type(Type type, std::any value) noexcept;
};

}  // namespace SimpleDBus
