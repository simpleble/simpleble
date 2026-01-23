#include <gtest/gtest.h>
#include <simpleble/Types.h>
#include <sstream>

TEST(BluetoothAddressTest, DefaultConstructor) {
    SimpleBLE::bluetoothAddress addr;
    EXPECT_EQ(addr.raw(), "");
    EXPECT_EQ(addr.to_string(), "");
}

TEST(BluetoothAddressTest, CStringConstructor) {
    SimpleBLE::bluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    EXPECT_EQ(addr.raw(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, CStringConstructorNull) {
    EXPECT_THROW(SimpleBLE::bluetoothAddress(static_cast<const char*>(nullptr)), std::invalid_argument);
}

TEST(BluetoothAddressTest, StdStringConstructor) {
    std::string s = "AA:BB:CC:DD:EE:FF";
    SimpleBLE::bluetoothAddress addr(s);
    EXPECT_EQ(addr.raw(), "AA:BB:CC:DD:EE:FF");
}

TEST(BluetoothAddressTest, StdStringMoveConstructor) {
    std::string s = "aa:bb:cc:dd:ee:ff";
    SimpleBLE::bluetoothAddress addr(std::move(s));
    EXPECT_EQ(addr.raw(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StringViewConstructor) {
    std::string_view sv = "12:34:56:78:9A:BC";
    SimpleBLE::bluetoothAddress addr(sv);
    EXPECT_EQ(addr.raw(), "12:34:56:78:9A:BC");
}

TEST(BluetoothAddressTest, ToStringLowercase) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(addr.to_string(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, ToStringMixedCase) {
    SimpleBLE::bluetoothAddress addr("aA:Bb:cC:dD:eE:fF");
    EXPECT_EQ(addr.to_string(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, ToStringAlreadyLowercase) {
    SimpleBLE::bluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    EXPECT_EQ(addr.to_string(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, ImplicitStringConversion) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = addr;
    EXPECT_EQ(str, "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, EqualitySameCase) {
    SimpleBLE::bluetoothAddress a("AA:BB:CC:DD:EE:FF");
    SimpleBLE::bluetoothAddress b("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(a == b);
}

TEST(BluetoothAddressTest, EqualityCaseInsensitive) {
    SimpleBLE::bluetoothAddress a("aa:bb:cc:dd:ee:ff");
    SimpleBLE::bluetoothAddress b("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(a == b);
}

TEST(BluetoothAddressTest, EqualityBothLowercase) {
    SimpleBLE::bluetoothAddress a("aa:bb:cc:dd:ee:ff");
    SimpleBLE::bluetoothAddress b("aa:bb:cc:dd:ee:ff");
    EXPECT_TRUE(a == b);
}

TEST(BluetoothAddressTest, EqualityMixedCase) {
    SimpleBLE::bluetoothAddress a("aA:bB:cC:dD:eE:fF");
    SimpleBLE::bluetoothAddress b("Aa:Bb:Cc:Dd:Ee:Ff");
    EXPECT_TRUE(a == b);
}

TEST(BluetoothAddressTest, InequalityDifferentAddresses) {
    SimpleBLE::bluetoothAddress a("AA:BB:CC:DD:EE:FF");
    SimpleBLE::bluetoothAddress b("11:22:33:44:55:66");
    EXPECT_FALSE(a == b);
}

TEST(BluetoothAddressTest, LessThanOperator) {
    SimpleBLE::bluetoothAddress a("11:22:33:44:55:66");
    SimpleBLE::bluetoothAddress b("AA:BB:CC:DD:EE:FF");
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(BluetoothAddressTest, LessThanCaseInsensitive) {
    SimpleBLE::bluetoothAddress a("aa:bb:cc:dd:ee:ff");
    SimpleBLE::bluetoothAddress b("AA:BB:CC:DD:EE:FF");
    // Same address, neither should be less than the other
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(BluetoothAddressTest, LessThanDifferentLengths) {
    SimpleBLE::bluetoothAddress a("AA:BB");
    SimpleBLE::bluetoothAddress b("AA:BB:CC");
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(BluetoothAddressTest, RawPreservesOriginalCase) {
    SimpleBLE::bluetoothAddress addr("aA:bB:cC:dD:eE:fF");
    EXPECT_EQ(addr.raw(), "aA:bB:cC:dD:eE:fF");
}

TEST(BluetoothAddressTest, StreamOperator) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::ostringstream oss;
    oss << addr;
    EXPECT_EQ(oss.str(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StreamOperatorLowercase) {
    SimpleBLE::bluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    std::ostringstream oss;
    oss << addr;
    EXPECT_EQ(oss.str(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StreamOperatorMixedCase) {
    SimpleBLE::bluetoothAddress addr("aA:bB:cC:dD:eE:fF");
    std::ostringstream oss;
    oss << addr;
    EXPECT_EQ(oss.str(), "aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StreamOperatorEmpty) {
    SimpleBLE::bluetoothAddress addr;
    std::ostringstream oss;
    oss << addr;
    EXPECT_EQ(oss.str(), "");
}

TEST(BluetoothAddressTest, StreamOperatorChained) {
    SimpleBLE::bluetoothAddress addr1("AA:BB:CC:DD:EE:FF");
    SimpleBLE::bluetoothAddress addr2("11:22:33:44:55:66");
    std::ostringstream oss;
    oss << addr1 << " - " << addr2;
    EXPECT_EQ(oss.str(), "aa:bb:cc:dd:ee:ff - 11:22:33:44:55:66");
}

TEST(BluetoothAddressTest, StringConcatenationStringPlusAddress) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = "Device: ";
    std::string result = str + addr;
    EXPECT_EQ(result, "Device: aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StringConcatenationAddressPlusString) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string str = " [Connected]";
    std::string result = addr + str;
    EXPECT_EQ(result, "aa:bb:cc:dd:ee:ff [Connected]");
}

TEST(BluetoothAddressTest, StringConcatenationCStringPlusAddress) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string result = "Device: " + addr;
    EXPECT_EQ(result, "Device: aa:bb:cc:dd:ee:ff");
}

TEST(BluetoothAddressTest, StringConcatenationAddressPlusCString) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string result = addr + " [Connected]";
    EXPECT_EQ(result, "aa:bb:cc:dd:ee:ff [Connected]");
}

TEST(BluetoothAddressTest, StringConcatenationChained) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    std::string result = "Device: " + addr + " [" + std::to_string(42) + "]";
    EXPECT_EQ(result, "Device: aa:bb:cc:dd:ee:ff [42]");
}

TEST(BluetoothAddressTest, StringConcatenationMixedCase) {
    SimpleBLE::bluetoothAddress addr("aA:Bb:cC:dD:eE:fF");
    std::string result = "Address: " + addr + " found";
    EXPECT_EQ(result, "Address: aa:bb:cc:dd:ee:ff found");
}

TEST(BluetoothAddressTest, StringConcatenationEmptyAddress) {
    SimpleBLE::bluetoothAddress addr;
    std::string result = "Empty: " + addr + " end";
    EXPECT_EQ(result, "Empty:  end");
}


