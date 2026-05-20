#include <gtest/gtest.h>

#include "simplecble/config.h"

TEST(ConfigTest, AndroidConnectionPriorityCanBeSetFromC) {
    simpleble_config_android_reset();
    EXPECT_EQ(simpleble_config_android_get_connection_priority(),
              SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_DISABLED);

    simpleble_config_android_set_connection_priority(SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_HIGH);
    EXPECT_EQ(simpleble_config_android_get_connection_priority(), SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_HIGH);

    simpleble_config_set_android_connection_priority(2);
    EXPECT_EQ(simpleble_config_android_get_connection_priority(),
              SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_LOW_POWER);

    simpleble_config_android_reset();
}

TEST(ConfigTest, SimpleBluezConfigCanBeSetFromC) {
    simpleble_config_simplebluez_reset();

    EXPECT_FALSE(simpleble_config_simplebluez_get_use_legacy_bluez_backend());

    simpleble_config_simplebluez_set_use_legacy_bluez_backend(true);
    simpleble_config_simplebluez_set_use_system_bus(false);
    simpleble_config_simplebluez_set_connection_timeout_ms(1234);
    simpleble_config_simplebluez_set_disconnection_timeout_ms(5678);

    EXPECT_TRUE(simpleble_config_simplebluez_get_use_legacy_bluez_backend());
    EXPECT_FALSE(simpleble_config_simplebluez_get_use_system_bus());
    EXPECT_EQ(simpleble_config_simplebluez_get_connection_timeout_ms(), 1234);
    EXPECT_EQ(simpleble_config_simplebluez_get_disconnection_timeout_ms(), 5678);

    simpleble_config_simplebluez_reset();
}

TEST(ConfigTest, WinRTConfigCanBeSetFromC) {
    simpleble_config_winrt_reset();

    simpleble_config_winrt_set_experimental_use_own_mta_apartment(false);
    simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(true);
    simpleble_config_winrt_set_use_deferred_disconnect(false);

    EXPECT_FALSE(simpleble_config_winrt_get_experimental_use_own_mta_apartment());
    EXPECT_TRUE(simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread());
    EXPECT_FALSE(simpleble_config_winrt_get_use_deferred_disconnect());

    simpleble_config_winrt_reset();
}

TEST(ConfigTest, DonglConfigCanBeSetFromC) {
    simpleble_config_dongl_reset();

    simpleble_config_dongl_set_use_dongl_backend(true);
    simpleble_config_dongl_set_auto_update(true);
    simpleble_config_dongl_set_force_update(true);

    EXPECT_TRUE(simpleble_config_dongl_get_use_dongl_backend());
    EXPECT_TRUE(simpleble_config_dongl_get_auto_update());
    EXPECT_TRUE(simpleble_config_dongl_get_force_update());

    simpleble_config_dongl_reset();
}
