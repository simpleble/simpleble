#include <gtest/gtest.h>
#include <simpleble/Types.h>
#include <sstream>

TEST(BluetoothAddressTest, DefaultConstructor) {
    SimpleBLE::BluetoothAddress addr;
    EXPECT_EQ(addr.raw(), "");
    EXPECT_EQ(addr.to_string(), "");
}

TEST(BluetoothAddressTest, CStringConstructor) {
    SimpleBLE::BluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    EXPECT_EQ(addr.raw(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, CStringConstructorNull) {
    EXPECT_THROW(SimpleBLE::BluetoothAddress(static_cast<const char*>(nullptr)), std::invalid_argument);
}

TEST(BluetoothAddressTest, StdStringConstructor) {
    std::string s = "AA:BB:CC:DD:EE:FF";
    SimpleBLE::BluetoothAddress addr(s);
    EXPECT_EQ(addr.raw(), "AA:BB:CC:DD:EE:FF");
}

TEST(BluetoothAddressTest, StdStringMoveConstructor) {
    std::string s = "aa:bb:cc:dd:ee:ff";
    SimpleBLE::BluetoothAddress addr(std::move(s));
    EXPECT_EQ(addr.raw(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StringViewConstructor) {
    std::string_view sv = "12:34:56:78:9A:BC";
    SimpleBLE::BluetoothAddress addr(sv);
    EXPECT_EQ(addr.raw(), "12:34:56:78:9A:BC");
}

TEST(BluetoothAddressTest, ToStringLowercase) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(addr.to_string(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, ToStringMixedCase) {
    SimpleBLE::BluetoothAddress addr("aA:Bb:cC:dD:eE:fF");
    EXPECT_EQ(addr.to_string(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, ToStringAlreadyLowercase) {
    SimpleBLE::BluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    EXPECT_EQ(addr.to_string(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, ImplicitStringConversion) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = addr;
    EXPECT_EQ(str, "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, EqualitySameCase) {
    SimpleBLE::BluetoothAddress a("AA:BB:CC:DD:EE:FF");
    SimpleBLE::BluetoothAddress b("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(a == b);
}

TEST(BluetoothAddressTest, EqualityCaseInsensitive) {
    SimpleBLE::BluetoothAddress a("aa:bb:cc:dd:ee:ff");
    SimpleBLE::BluetoothAddress b("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(a == b);
}

TEST(BluetoothAddressTest, EqualityBothLowercase) {
    SimpleBLE::BluetoothAddress a("aa:bb:cc:dd:ee:ff");
    SimpleBLE::BluetoothAddress b("aa:bb:cc:dd:ee:ff");
    EXPECT_TRUE(a == b);
}

TEST(BluetoothAddressTest, EqualityMixedCase) {
    SimpleBLE::BluetoothAddress a("aA:bB:cC:dD:eE:fF");
    SimpleBLE::BluetoothAddress b("Aa:Bb:Cc:Dd:Ee:Ff");
    EXPECT_TRUE(a == b);
}

TEST(BluetoothAddressTest, InequalityDifferentAddresses) {
    SimpleBLE::BluetoothAddress a("AA:BB:CC:DD:EE:FF");
    SimpleBLE::BluetoothAddress b("11:22:33:44:55:66");
    EXPECT_FALSE(a == b);
}

TEST(BluetoothAddressTest, LessThanOperator) {
    SimpleBLE::BluetoothAddress a("11:22:33:44:55:66");
    SimpleBLE::BluetoothAddress b("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(BluetoothAddressTest, LessThanCaseInsensitive) {
    SimpleBLE::BluetoothAddress a("aa:bb:cc:dd:ee:ff");
    SimpleBLE::BluetoothAddress b("AA:BB:CC:DD:EE:FF");
    // Same address, neither should be less than the other
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(BluetoothAddressTest, LessThanDifferentLengths) {
    SimpleBLE::BluetoothAddress a("AA:BB");
    SimpleBLE::BluetoothAddress b("AA:BB:CC");
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(BluetoothAddressTest, RawPreservesOriginalCase) {
    SimpleBLE::BluetoothAddress addr("aA:bB:cC:dD:eE:fF");
    EXPECT_EQ(addr.raw(), "aA:bB:cC:dD:eE:fF");
}

TEST(BluetoothAddressTest, StreamOperator) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::ostringstream oss;
    oss << addr;
    EXPECT_EQ(oss.str(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StreamOperatorLowercase) {
    SimpleBLE::BluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    std::ostringstream oss;
    oss << addr;
    EXPECT_EQ(oss.str(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StreamOperatorMixedCase) {
    SimpleBLE::BluetoothAddress addr("aA:bB:cC:dD:eE:fF");
    std::ostringstream oss;
    oss << addr;
    EXPECT_EQ(oss.str(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StreamOperatorEmpty) {
    SimpleBLE::BluetoothAddress addr;
    std::ostringstream oss;
    oss << addr;
    EXPECT_EQ(oss.str(), "");
}

TEST(BluetoothAddressTest, StreamOperatorChained) {
    SimpleBLE::BluetoothAddress addr1("AA:BB:CC:DD:EE:FF");
    SimpleBLE::BluetoothAddress addr2("11:22:33:44:55:66");
    std::ostringstream oss;
    oss << addr1 << " - " << addr2;
    EXPECT_EQ(oss.str(), "aa:bb:cc:dd:ee:ff - 11:22:33:44:55:66");
}

TEST(BluetoothAddressTest, StringConcatenationStringPlusAddress) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "Device: ";
    std::string result = str + addr;
    EXPECT_EQ(result, "Device: aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StringConcatenationAddressPlusString) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = " [Connected]";
    std::string result = addr + str;
    EXPECT_EQ(result, "aa:bb:cc:dd:ee:ff [Connected]");
}

TEST(BluetoothAddressTest, StringConcatenationCStringPlusAddress) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string result = "Device: " + addr;
    EXPECT_EQ(result, "Device: aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StringConcatenationAddressPlusCString) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string result = addr + " [Connected]";
    EXPECT_EQ(result, "aa:bb:cc:dd:ee:ff [Connected]");
}

TEST(BluetoothAddressTest, StringConcatenationChained) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string result = "Device: " + addr + " [" + std::to_string(42) + "]";
    EXPECT_EQ(result, "Device: aa:bb:cc:dd:ee:ff [42]");
}

TEST(BluetoothAddressTest, StringConcatenationMixedCase) {
    SimpleBLE::BluetoothAddress addr("aA:Bb:cC:dD:eE:fF");
    std::string result = "Address: " + addr + " found";
    EXPECT_EQ(result, "Address: aa:bb:cc:dd:ee:ff found");
}

TEST(BluetoothAddressTest, StringConcatenationEmptyAddress) {
    SimpleBLE::BluetoothAddress addr;
    std::string result = "Empty: " + addr + " end";
    EXPECT_EQ(result, "Empty:  end");
}

TEST(BluetoothAddressTest, EqualityStringEqualsAddress) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "aa:bb:cc:dd:ee:ff";
    EXPECT_TRUE(str == addr);
}

TEST(BluetoothAddressTest, EqualityAddressEqualsString) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "aa:bb:cc:dd:ee:ff";
    EXPECT_TRUE(addr == str);
}

TEST(BluetoothAddressTest, EqualityStringEqualsAddressCaseInsensitive) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "AA:BB:CC:DD:EE:FF";
    EXPECT_TRUE(str == addr);
}

TEST(BluetoothAddressTest, EqualityAddressEqualsStringCaseInsensitive) {
    SimpleBLE::BluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    std::string str = "AA:BB:CC:DD:EE:FF";
    EXPECT_TRUE(addr == str);
}

TEST(BluetoothAddressTest, EqualityStringEqualsAddressMixedCase) {
    SimpleBLE::BluetoothAddress addr("aA:Bb:cC:dD:eE:fF");
    std::string str = "Aa:Bb:Cc:Dd:Ee:Ff";
    EXPECT_TRUE(str == addr);
}

TEST(BluetoothAddressTest, InequalityStringNotEqualsAddress) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "11:22:33:44:55:66";
    EXPECT_TRUE(str != addr);
}

TEST(BluetoothAddressTest, InequalityAddressNotEqualsString) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "11:22:33:44:55:66";
    EXPECT_TRUE(addr != str);
}

TEST(BluetoothAddressTest, InequalityStringNotEqualsAddressSameValue) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "aa:bb:cc:dd:ee:ff";
    EXPECT_FALSE(str != addr);
}

TEST(BluetoothAddressTest, InequalityAddressNotEqualsStringSameValue) {
    SimpleBLE::BluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "aa:bb:cc:dd:ee:ff";
    EXPECT_FALSE(addr != str);
}


