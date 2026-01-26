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
    std::map<uint16_t, std::vector<uint8_t>> value = {{1, {0x11, 0x12}}, {2, {0x21, 0x22}}};
    Holder h = HolderUtils::from_dict_uint16_byte_array(value);

    EXPECT_EQ(h.type(), Holder::Type::DICT);
    EXPECT_EQ(h.signature(), "a{qv}");

    std::map<uint16_t, std::vector<uint8_t>> result = HolderUtils::to_dict_uint16_byte_array(h);
    EXPECT_EQ(value, result);
}

TEST(HolderUtils, DictStringByteArray) {
    std::map<std::string, std::vector<uint8_t>> value = {{"key1", {0x31, 0x32}}, {"key2", {0x41, 0x42}}};
    Holder h = HolderUtils::from_dict_string_byte_array(value);

    EXPECT_EQ(h.type(), Holder::Type::DICT);
    EXPECT_EQ(h.signature(), "a{sv}");

    std::map<std::string, std::vector<uint8_t>> result = HolderUtils::to_dict_string_byte_array(h);
    EXPECT_EQ(value, result);
}
