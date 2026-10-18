#include <simplektble/logging.h>

#include <simpleble/Logging.h>

void simplektble_log_error(const char* function, const char* message) {
    try {
        SimpleBLE::Logging::Logger::get()->log(SimpleBLE::Logging::Level::Error, "SimpleKtBLE", __FILE__, __LINE__,
                                               function != nullptr ? function : "", message != nullptr ? message : "");
    } catch (...) {
        // NOTE: Exceptions must not cross back into Kotlin.
    }
}
