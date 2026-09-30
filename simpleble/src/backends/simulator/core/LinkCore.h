#pragma once

#include <cstdint>
#include <mutex>

namespace SimpleBLE::Simulation::Internal {

/** Radio conditions between one adapter and one device. */
class LinkCore {
  public:
    mutable std::mutex mutex;
    bool in_range = true;
    float packet_error_rate = 0.0f;
    int16_t rssi = -60;
};

}  // namespace SimpleBLE::Simulation::Internal
