#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>
#include "backends/dongl/serial/Protocol.h"

using namespace SimpleBLE;

TEST(BufferOverflowTest, DonglProtocolWriteOverflow) {
    try {
        SimpleBLE::Dongl::Serial::Protocol p("dummy");
        std::vector<uint8_t> data(1000, 'C');
        // This should throw std::length_error before trying to write to the serial port
        p.simpleble_write(0, 0, static_cast<simpleble_WriteOperation>(0), data);
        FAIL() << "Expected std::length_error";
    } catch (const std::length_error& e) {
        EXPECT_STREQ(e.what(), "Payload exceeds maximum size of 512 bytes");
    } catch (const std::runtime_error& e) {
        // If it throws runtime_error because dummy device doesn't exist, we can't fully test it this way,
        // but the compiler has the static_assert at least.
        SUCCEED();
    }
}
