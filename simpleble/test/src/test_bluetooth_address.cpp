#include <gtest/gtest.h>
#include <simpleble/Types.h>

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

TEST(BluetoothAddressTest, ToStringUppercase) {
    SimpleBLE::bluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    EXPECT_EQ(addr.to_string(), "AA:BB:CC:DD:EE:FF");
}

TEST(BluetoothAddressTest, ToStringMixedCase) {
    SimpleBLE::bluetoothAddress addr("aA:Bb:cC:dD:eE:fF");
    EXPECT_EQ(addr.to_string(), "AA:BB:CC:DD:EE:FF");
}

TEST(BluetoothAddressTest, ToStringAlreadyUppercase) {
    SimpleBLE::bluetoothAddress addr("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(addr.to_string(), "AA:BB:CC:DD:EE:FF");
}

TEST(BluetoothAddressTest, ImplicitStringConversion) {
    SimpleBLE::bluetoothAddress addr("aa:bb:cc:dd:ee:ff");
    std::string str = addr;
    EXPECT_EQ(str, "AA:BB:CC:DD:EE:FF");
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


