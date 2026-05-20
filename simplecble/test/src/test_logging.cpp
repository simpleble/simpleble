#include <gtest/gtest.h>

#include "simplecble/logging.h"

TEST(LoggingTest, LevelCanBeReadBackFromC) {
    simpleble_logging_set_level(SIMPLEBLE_LOG_LEVEL_DEBUG);
    EXPECT_EQ(simpleble_logging_get_level(), SIMPLEBLE_LOG_LEVEL_DEBUG);

    simpleble_logging_set_level(SIMPLEBLE_LOG_LEVEL_INFO);
}

TEST(LoggingTest, CallbackCanBeClearedFromC) {
    simpleble_logging_set_callback(nullptr);
    EXPECT_FALSE(simpleble_logging_has_callback());
}
