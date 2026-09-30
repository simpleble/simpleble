#include "ConnectionCore.h"

namespace SimpleBLE::Simulation::Internal {

namespace {

void enqueue(std::deque<Pdu>& queue, Pdu pdu) {
    pdu.bytes_left = pdu.wire_length();
    if (pdu.type == PduType::TERMINATE) {
        queue.push_front(std::move(pdu));
    } else {
        queue.push_back(std::move(pdu));
    }
}

}  // namespace

bool ConnectionCore::is_open() const {
    std::lock_guard<std::mutex> lock(mutex);
    return open;
}

bool ConnectionCore::send_to_device(Pdu pdu) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!open) return false;
    enqueue(to_device, std::move(pdu));
    return true;
}

bool ConnectionCore::send_to_central(Pdu pdu) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!open) return false;
    enqueue(to_central, std::move(pdu));
    return true;
}

void ConnectionCore::enqueue_to_device_locked(Pdu pdu) { enqueue(to_device, std::move(pdu)); }

void ConnectionCore::enqueue_to_central_locked(Pdu pdu) { enqueue(to_central, std::move(pdu)); }

}  // namespace SimpleBLE::Simulation::Internal
