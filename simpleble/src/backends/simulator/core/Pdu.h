#pragma once

#include <simpleble/Types.h>
#include <simpleble/simulation/Types.h>

#include <cstddef>
#include <functional>

#include "SimulatorUtils.h"

namespace SimpleBLE::Simulation::Internal {

enum class PduType {
    // Central to device
    MTU_REQUEST,
    DISCOVERY_REQUEST,
    READ,
    WRITE_REQUEST,
    WRITE_COMMAND,
    READ_DESCRIPTOR,
    WRITE_DESCRIPTOR,
    CONFIRMATION,
    CONNECTION_PARAMETER_RESPONSE,  // L2CAP signaling
    CONNECTION_UPDATE,              // LL control
    // Device to central
    RESPONSE,
    NOTIFICATION,
    INDICATION,
    CONNECTION_PARAMETER_REQUEST,  // L2CAP signaling
    // Either direction
    TERMINATE,  // LL control
};

using ResponseHandler = std::function<void(AttStatus status, ByteArray value)>;

struct Pdu {
    PduType type;
    BluetoothUUID service;
    BluetoothUUID characteristic;
    BluetoothUUID descriptor;
    ByteArray value;
    AttStatus status = AttStatus::SUCCESS;
    Clock::duration interval_min{};
    Clock::duration interval_max{};
    /** Link-layer payload bytes still to be sent, across as many packets as needed. */
    size_t bytes_left = 0;
    bool started = false;
    bool uses_tx_buffer = false;
    /**
     * Carried by a request and then by its response; called on the air thread
     * when the response reaches the central. Dropping it unanswered (because
     * the connection closed) breaks the promise it captures.
     */
    ResponseHandler on_response;

    bool is_request() const;
    /** Bytes this PDU occupies in link-layer payloads, including the L2CAP header. */
    size_t wire_length() const;
};

}  // namespace SimpleBLE::Simulation::Internal
