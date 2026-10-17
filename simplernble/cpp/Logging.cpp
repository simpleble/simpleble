#include "Logging.hpp"

#include <simpleble/Logging.h>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace margelo::nitro::simplernble {

void log_to_system() {
    using SimpleBLE::Logging::Level;
#if defined(__ANDROID__)
    SimpleBLE::Logging::Logger::get()->set_callback([](Level level, const std::string& module, const std::string& file,
                                                       uint32_t line, const std::string& function,
                                                       const std::string& message) {
        int priority = ANDROID_LOG_INFO;
        switch (level) {
            case Level::Verbose:
                priority = ANDROID_LOG_VERBOSE;
                break;
            case Level::Debug:
                priority = ANDROID_LOG_DEBUG;
                break;
            case Level::Warn:
                priority = ANDROID_LOG_WARN;
                break;
            case Level::Error:
                priority = ANDROID_LOG_ERROR;
                break;
            case Level::Fatal:
                priority = ANDROID_LOG_FATAL;
                break;
            default:
                break;
        }
        __android_log_print(priority, "SimpleBLE", "%s: %s:%u in %s: %s", module.c_str(), file.c_str(), line,
                            function.c_str(), message.c_str());
    });
#else
    SimpleBLE::Logging::Logger::get()->log_default_stdout();
#endif
}

}  // namespace margelo::nitro::simplernble
