#include <simpleble/simulation/Connection.h>

#include <mutex>

#include "core/ConnectionCore.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Simulation;
using namespace SimpleBLE::Simulation::Internal;

Connection::Connection(std::shared_ptr<ConnectionCore> internal) : internal_(std::move(internal)) {}

bool Connection::is_connected() const {
    auto connection = internal_.lock();
    return connection && connection->is_open();
}

uint16_t Connection::mtu() const {
    auto connection = internal_.lock();
    if (!connection) return 0;
    std::lock_guard<std::mutex> lock(connection->mutex);
    return connection->mtu;
}

std::chrono::microseconds Connection::interval() const {
    auto connection = internal_.lock();
    if (!connection) return {};
    std::lock_guard<std::mutex> lock(connection->mutex);
    return std::chrono::duration_cast<std::chrono::microseconds>(connection->interval);
}

bool Connection::operator==(const Connection& other) const {
    return !internal_.owner_before(other.internal_) && !other.internal_.owner_before(internal_);
}

std::shared_ptr<ConnectionCore> Connection::internal() const { return internal_.lock(); }
