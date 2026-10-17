#pragma once

namespace margelo::nitro::simplernble {

// Sends SimpleBLE's log to the system's: logcat on Android, standard output
// elsewhere. This is the default.
void log_to_system();

}  // namespace margelo::nitro::simplernble
