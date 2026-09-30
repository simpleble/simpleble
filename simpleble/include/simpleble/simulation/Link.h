#pragma once

#include <cstdint>
#include <memory>

#include <simpleble/export.h>

namespace SimpleBLE::Simulation::Internal {
class LinkCore;
}

namespace SimpleBLE::Simulation {

/**
 * Radio conditions between one adapter and one device. Changes take effect
 * immediately, including on an open connection.
 */
class SIMPLEBLE_EXPORT Link {
  public:
    Link() = default;
    explicit Link(std::shared_ptr<Internal::LinkCore> internal);

    /** Out of range, neither side hears the other. Default: true. */
    void set_in_range(bool in_range);
    bool is_in_range() const;

    /**
     * Probability that a packet is corrupted, from 0 to 1. Corrupted packets
     * are sent again, and two in a row end the connection event. Default: 0.
     */
    void set_packet_error_rate(float rate);
    float packet_error_rate() const;

    /** Default: -60 dBm. */
    void set_rssi(int16_t rssi);
    int16_t rssi() const;

  private:
    std::shared_ptr<Internal::LinkCore> internal_;
};

}  // namespace SimpleBLE::Simulation
