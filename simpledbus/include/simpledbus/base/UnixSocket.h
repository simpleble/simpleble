#pragma once

#include <sys/socket.h>
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace SimpleDBus {

class UnixSocket {
  public:
    enum class Type { STREAM, DATAGRAM, SEQPACKET };

    UnixSocket() = default;
    explicit UnixSocket(int fd);
    ~UnixSocket();

    UnixSocket(const UnixSocket&) = delete;
    UnixSocket& operator=(const UnixSocket&) = delete;

    UnixSocket(UnixSocket&& other) noexcept;
    UnixSocket& operator=(UnixSocket&& other) noexcept;

    static std::pair<UnixSocket, UnixSocket> create_pair(Type type = Type::SEQPACKET, bool non_blocking = true,
                                                         bool close_on_exec = true);

    int fd() const;
    bool valid() const;
    int release();
    void close();

    ssize_t send(const void* data, size_t size, int flags = MSG_NOSIGNAL);
    ssize_t send(const std::vector<uint8_t>& data, int flags = MSG_NOSIGNAL);
    ssize_t receive(void* data, size_t size, int flags = 0);

  private:
    int _fd = -1;
};

}  // namespace SimpleDBus
