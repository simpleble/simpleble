#include <gtest/gtest.h>

#include <simpledbus/base/UnixSocket.h>

#include <array>
#include <vector>
#include <unistd.h>

using namespace SimpleDBus;

TEST(UnixSocketTest, CreatePairAndExchangeBytes) {
    auto sockets = UnixSocket::create_pair();

    std::vector<uint8_t> outgoing{0x01, 0x02, 0x03};
    std::array<uint8_t, 3> incoming{};

    EXPECT_TRUE(sockets.first.valid());
    EXPECT_TRUE(sockets.second.valid());
    EXPECT_EQ(sockets.first.send(outgoing), static_cast<ssize_t>(outgoing.size()));
    EXPECT_EQ(sockets.second.receive(incoming.data(), incoming.size()), static_cast<ssize_t>(incoming.size()));
    EXPECT_EQ(std::vector<uint8_t>(incoming.begin(), incoming.end()), outgoing);
}

TEST(UnixSocketTest, MoveTransfersOwnership) {
    auto sockets = UnixSocket::create_pair();
    int fd = sockets.first.fd();

    UnixSocket moved(std::move(sockets.first));

    EXPECT_FALSE(sockets.first.valid());
    EXPECT_TRUE(moved.valid());
    EXPECT_EQ(moved.fd(), fd);

    int released_fd = moved.release();
    EXPECT_FALSE(moved.valid());
    EXPECT_EQ(released_fd, fd);

    ::close(released_fd);
}
