#pragma once

#include <cstdint>
#include <utility>

#include <simpleble/Types.h>

namespace SimpleBLE::Simulation {

/** Characteristic properties that SimpleBLE exposes to the central. */
enum class Property { READ, WRITE_REQUEST, WRITE_COMMAND, NOTIFY, INDICATE };

enum class SubscriptionKind { NOTIFY, INDICATE };

/**
 * ATT status returned by a device to a request.
 *
 * Application errors (0x80-0x9F) can be returned with a static_cast.
 */
enum class AttStatus : uint8_t {
    SUCCESS = 0x00,
    INVALID_HANDLE = 0x01,
    READ_NOT_PERMITTED = 0x02,
    WRITE_NOT_PERMITTED = 0x03,
    INSUFFICIENT_AUTHORIZATION = 0x08,
    INVALID_ATTRIBUTE_VALUE_LENGTH = 0x0D,
    UNLIKELY_ERROR = 0x0E,
    VALUE_NOT_ALLOWED = 0x13,
};

/** Result of queueing a notification or indication. */
enum class TxStatus {
    QUEUED,          ///< Queued for the next connection events.
    BUSY,            ///< TX buffers are full, or an indication awaits confirmation.
    NOT_SUBSCRIBED,  ///< The central has not subscribed to this characteristic.
    NOT_CONNECTED,   ///< The connection has closed.
};

/** Reply to a read: a value, or an ATT error. */
struct ReadResult {
    ReadResult() = default;
    ReadResult(ByteArray value) : value(std::move(value)) {}
    ReadResult(AttStatus status) : status(status) {}

    AttStatus status = AttStatus::SUCCESS;
    ByteArray value;
};

}  // namespace SimpleBLE::Simulation
