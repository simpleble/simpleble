#include <simpleble/Logging.h>
#include <simplecble/logging.h>

void simpleble_logging_set_level(simpleble_log_level_t level) {
    SimpleBLE::Logging::Logger::get()->set_level(static_cast<SimpleBLE::Logging::Level>(level));
}

simpleble_log_level_t simpleble_logging_get_level(void) {
    return static_cast<simpleble_log_level_t>(SimpleBLE::Logging::Logger::get()->get_level());
}

void simpleble_logging_set_callback(simpleble_log_callback_t callback) {
    if (callback == nullptr) {
        SimpleBLE::Logging::Logger::get()->set_callback(nullptr);
        return;
    }

    SimpleBLE::Logging::Logger::get()->set_callback(
        [callback](SimpleBLE::Logging::Level level, const std::string& module, const std::string& file, uint32_t line,
                   const std::string& function, const std::string& message) {
            try {
                callback(static_cast<simpleble_log_level_t>(level), module.c_str(), file.c_str(), line,
                         function.c_str(), message.c_str());
            } catch (...) {
                // Clearly, if the logging callback throws an exception, we should not crash.
            }
        });
}

bool simpleble_logging_has_callback(void) { return SimpleBLE::Logging::Logger::get()->has_callback(); }

void simpleble_logging_log_default_stdout(void) { SimpleBLE::Logging::Logger::get()->log_default_stdout(); }

void simpleble_logging_log_default_file(void) { SimpleBLE::Logging::Logger::get()->log_default_file(); }

void simpleble_logging_log_default_file_path(const char* path) {
    if (path == nullptr) {
        SimpleBLE::Logging::Logger::get()->log_default_file();
        return;
    }

    SimpleBLE::Logging::Logger::get()->log_default_file(path);
}
