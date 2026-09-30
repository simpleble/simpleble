#pragma once

#include <simpleble/Types.h>

#include <chrono>
#include <functional>
#include <utility>
#include <vector>

namespace SimpleBLE::Simulation::Internal {

using Clock = std::chrono::steady_clock;
using Actions = std::vector<std::function<void()>>;

extern const BluetoothUUID CCCD_UUID;

BluetoothUUID normalize(const BluetoothUUID& uuid);

/** Clamps a connection interval to the 7.5 ms to 4 s range of the Bluetooth Core specification. */
Clock::duration clamp_interval(Clock::duration interval);

/** Clamps both ends to valid intervals and keeps the range non-empty. */
std::pair<Clock::duration, Clock::duration> interval_range(Clock::duration min, Clock::duration max);

}  // namespace SimpleBLE::Simulation::Internal
