#include <simpledbus/base/UnixSocket.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>

using namespace SimpleDBus;

namespace {

int socket_type(UnixSocket::Type type) {
    switch (type) {
        case UnixSocket::Type::STREAM:
            return SOCK_STREAM;
        case UnixSocket::Type::DATAGRAM:
            return SOCK_DGRAM;
        case UnixSocket::Type::SEQPACKET:
            return SOCK_SEQPACKET;
    }

    return SOCK_SEQPACKET;
}

std::runtime_error socket_error(const std::string& operation) {
    return std::runtime_error(operation + ": " + std::strerror(errno));
}

}  // namespace

UnixSocket::UnixSocket(int fd) : _fd(fd) {}

UnixSocket::~UnixSocket() { close(); }

UnixSocket::UnixSocket(UnixSocket&& other) noexcept : _fd(other.release()) {}

UnixSocket& UnixSocket::operator=(UnixSocket&& other) noexcept {
    if (this != &other) {
        close();
        _fd = other.release();
    }

    return *this;
}

std::pair<UnixSocket, UnixSocket> UnixSocket::create_pair(Type type, bool non_blocking, bool close_on_exec) {
    int socket_flags = socket_type(type);
    if (non_blocking) {
        socket_flags |= SOCK_NONBLOCK;
    }
    if (close_on_exec) {
        socket_flags |= SOCK_CLOEXEC;
    }

    int fds[2] = {-1, -1};
    if (::socketpair(AF_LOCAL, socket_flags, 0, fds) < 0) {
        throw socket_error("socketpair");
    }

    return {UnixSocket(fds[0]), UnixSocket(fds[1])};
}

int UnixSocket::fd() const { return _fd; }

bool UnixSocket::valid() const { return _fd >= 0; }

int UnixSocket::release() {
    int fd = _fd;
    _fd = -1;
    return fd;
}

void UnixSocket::close() {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
}

ssize_t UnixSocket::send(const void* data, size_t size, int flags) {
    if (_fd < 0) {
        errno = EBADF;
        return -1;
    }

    return ::send(_fd, data, size, flags);
}

ssize_t UnixSocket::send(const std::vector<uint8_t>& data, int flags) { return send(data.data(), data.size(), flags); }

ssize_t UnixSocket::receive(void* data, size_t size, int flags) {
    if (_fd < 0) {
        errno = EBADF;
        return -1;
    }

    return ::recv(_fd, data, size, flags);
}
