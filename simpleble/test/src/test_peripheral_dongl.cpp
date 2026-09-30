#include <gtest/gtest.h>

#include "backends/dongl/PeripheralDongl.h"

#include <chrono>
#include <cstring>
#include <future>
#include <string>
#include <thread>

using namespace std::chrono_literals;

TEST(PeripheralDongl, PasskeyDisplayRunsOffEventThread) {
    std::promise<std::thread::id> displayed;
    auto display_thread = displayed.get_future();

    SimpleBLE::PeripheralDongl peripheral(nullptr, {});
    peripheral.set_passkey_display_callback(
        [&](const std::string& passkey) {
            EXPECT_EQ(passkey, "123456");
            displayed.set_value(std::this_thread::get_id());
        });

    simpleble_PasskeyDisplayEvt evt{};
    std::strcpy(evt.passkey, "123456");
    evt.match_request = false;
    peripheral.notify_passkey_display(evt);

    ASSERT_EQ(display_thread.wait_for(2s), std::future_status::ready);
    EXPECT_NE(display_thread.get(), std::this_thread::get_id());
}
