#include "Pdu.h"

namespace SimpleBLE::Simulation::Internal {

namespace {

constexpr size_t L2CAP_HEADER = 4;

}  // namespace

bool Pdu::is_request() const {
    switch (type) {
        case PduType::MTU_REQUEST:
        case PduType::DISCOVERY_REQUEST:
        case PduType::READ:
        case PduType::WRITE_REQUEST:
        case PduType::READ_DESCRIPTOR:
        case PduType::WRITE_DESCRIPTOR:
            return true;
        default:
            return false;
    }
}

size_t Pdu::wire_length() const {
    switch (type) {
        case PduType::TERMINATE:
            return 2;  // LL_TERMINATE_IND: opcode and error code
        case PduType::CONNECTION_UPDATE:
            return 12;  // LL_CONNECTION_UPDATE_IND
        case PduType::CONNECTION_PARAMETER_REQUEST:
            return L2CAP_HEADER + 4 + 8;  // Signaling header and interval, latency and timeout
        case PduType::CONNECTION_PARAMETER_RESPONSE:
            return L2CAP_HEADER + 4 + 2;  // Signaling header and result
        case PduType::CONFIRMATION:
            return L2CAP_HEADER + 1;
        case PduType::RESPONSE:
            return L2CAP_HEADER + 1 + value.size();
        case PduType::DISCOVERY_REQUEST:
            return L2CAP_HEADER + 7;  // Read By Group Type / Read By Type request
        default:
            return L2CAP_HEADER + 3 + value.size();  // Opcode, handle and value
    }
}

}  // namespace SimpleBLE::Simulation::Internal
