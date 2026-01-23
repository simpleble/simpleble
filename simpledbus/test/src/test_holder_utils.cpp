#include <gtest/gtest.h>
#include <simpledbus/base/HolderUtils.h>

using namespace SimpleDBus;

TEST(HolderUtils, ByteArray) {
    std::vector<uint8_t> value = {0x12, 0x34, 0x56, 0x78};
    Holder h = HolderUtils::from_byte_array(value);

    EXPECT_EQ(h.type(), Holder::Type::ARRAY);
    EXPECT_EQ(h.signature(), "ay");

    std::vector<uint8_t> result = HolderUtils::to_byte_array(h);
    EXPECT_EQ(value, result);
}

TEST(HolderUtils, StringArray) {
    std::vector<std::string> value = {"one", "two", "three"};
    Holder h = HolderUtils::from_string_array(value);

    EXPECT_EQ(h.type(), Holder::Type::ARRAY);
    EXPECT_EQ(h.signature(), "as");

    std::vector<std::string> result = HolderUtils::to_string_array(h);
    EXPECT_EQ(value, result);
}

TEST(HolderUtils, DictUint16ByteArray) {
    std::map<uint16_t, kvn::bytearray> value = {{1, {0x11, 0x12}}, {2, {0x21, 0x22}}};
    Holder h = HolderUtils::from_dict_uint16_byte_array(value);

    EXPECT_EQ(h.type(), Holder::Type::DICT);
    EXPECT_EQ(h.signature(), "a{qv}");

    std::map<uint16_t, kvn::bytearray> result = HolderUtils::to_dict_uint16_byte_array(h);
    EXPECT_EQ(value.size(), result.size());
    EXPECT_EQ(value.at(1).toHex(), result.at(1).toHex());
    EXPECT_EQ(value.at(2).toHex(), result.at(2).toHex());
}

TEST(HolderUtils, DictStringByteArray) {
    std::map<std::string, kvn::bytearray> value = {{"key1", {0x31, 0x32}}, {"key2", {0x41, 0x42}}};
    Holder h = HolderUtils::from_dict_string_byte_array(value);

    EXPECT_EQ(h.type(), Holder::Type::DICT);
    EXPECT_EQ(h.signature(), "a{sv}");

    std::map<std::string, kvn::bytearray> result = HolderUtils::to_dict_string_byte_array(h);
    EXPECT_EQ(value.size(), result.size());
    EXPECT_EQ(value.at("key1").toHex(), result.at("key1").toHex());
    EXPECT_EQ(value.at("key2").toHex(), result.at("key2").toHex());
}
