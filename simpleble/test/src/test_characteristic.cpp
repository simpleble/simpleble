#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <simpleble/Characteristic.h>

#include "backends/common/CharacteristicBase.h"

namespace {

class TestCharacteristic : public SimpleBLE::Characteristic {
  public:
    TestCharacteristic(bool can_broadcast, bool can_write_authenticated_signed, bool has_extended_properties) {
        internal_ = std::make_shared<SimpleBLE::CharacteristicBase>(
            "00002A00-0000-1000-8000-00805F9B34FB", std::vector<std::shared_ptr<SimpleBLE::DescriptorBase>>{}, false,
            false, false, false, false, can_broadcast, can_write_authenticated_signed, has_extended_properties);
    }
};

TEST(CharacteristicTest, AdditionalPropertyAccessorsAndCapabilities) {
    TestCharacteristic characteristic(true, true, true);

    EXPECT_TRUE(characteristic.can_broadcast());
    EXPECT_TRUE(characteristic.can_write_authenticated_signed());
    EXPECT_TRUE(characteristic.has_extended_properties());
    EXPECT_EQ(characteristic.capabilities(),
              std::vector<std::string>({"broadcast", "authenticated_signed_writes", "extended_properties"}));
}

TEST(CharacteristicTest, AdditionalCapabilitiesAreOmittedWhenUnsupported) {
    TestCharacteristic characteristic(false, false, false);

    EXPECT_FALSE(characteristic.can_broadcast());
    EXPECT_FALSE(characteristic.can_write_authenticated_signed());
    EXPECT_FALSE(characteristic.has_extended_properties());
    EXPECT_TRUE(characteristic.capabilities().empty());
}

}  // namespace