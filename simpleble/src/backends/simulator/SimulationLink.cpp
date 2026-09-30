#include <simpleble/simulation/Link.h>

#include <simpleble/Exceptions.h>

#include <algorithm>
#include <mutex>

#include "core/LinkCore.h"

using namespace SimpleBLE;
using namespace SimpleBLE::Simulation;
using namespace SimpleBLE::Simulation::Internal;

namespace {

LinkCore& link_core(const std::shared_ptr<LinkCore>& internal) {
    if (!internal) throw Exception::NotInitialized();
    return *internal;
}

}  // namespace

Link::Link(std::shared_ptr<LinkCore> internal) : internal_(std::move(internal)) {}

void Link::set_in_range(bool in_range) {
    auto& link = link_core(internal_);
    std::lock_guard<std::mutex> lock(link.mutex);
    link.in_range = in_range;
}

bool Link::is_in_range() const {
    auto& link = link_core(internal_);
    std::lock_guard<std::mutex> lock(link.mutex);
    return link.in_range;
}

void Link::set_packet_error_rate(float rate) {
    auto& link = link_core(internal_);
    std::lock_guard<std::mutex> lock(link.mutex);
    link.packet_error_rate = std::clamp(rate, 0.0f, 1.0f);
}

float Link::packet_error_rate() const {
    auto& link = link_core(internal_);
    std::lock_guard<std::mutex> lock(link.mutex);
    return link.packet_error_rate;
}

void Link::set_rssi(int16_t rssi) {
    auto& link = link_core(internal_);
    std::lock_guard<std::mutex> lock(link.mutex);
    link.rssi = rssi;
}

int16_t Link::rssi() const {
    auto& link = link_core(internal_);
    std::lock_guard<std::mutex> lock(link.mutex);
    return link.rssi;
}
