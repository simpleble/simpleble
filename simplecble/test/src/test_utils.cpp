#include <gtest/gtest.h>

#include "simplecble/utils.h"

TEST(UtilsTest, OperatingSystemIsKnown) {
    EXPECT_NE(simpleble_get_operating_system(), SIMPLEBLE_OS_UNKNOWN);
}
