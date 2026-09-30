#include "SimulatorUtils.h"

#include <algorithm>
#include <cctype>

namespace SimpleBLE::Simulation::Internal {

const BluetoothUUID CCCD_UUID = "00002902-0000-1000-8000-00805f9b34fb";

BluetoothUUID normalize(const BluetoothUUID& uuid) {
    BluetoothUUID result = uuid;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

Clock::duration clamp_interval(Clock::duration interval) {
    return std::clamp<Clock::duration>(interval, std::chrono::microseconds(7500), std::chrono::seconds(4));
}

std::pair<Clock::duration, Clock::duration> interval_range(Clock::duration min, Clock::duration max) {
    auto low = clamp_interval(min);
    auto high = std::max(low, clamp_interval(max));
    return {low, high};
}

}  // namespace SimpleBLE::Simulation::Internal
