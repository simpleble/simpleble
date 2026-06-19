#pragma once

#include <cstdint>
#include <string>

namespace SimpleDBus {

// clang-format off

void log_fatal(const std::string& file, uint32_t line, const std::string& function, const std::string& message);
void log_error(const std::string& file, uint32_t line, const std::string& function, const std::string& message);
void log_warn(const std::string& file, uint32_t line, const std::string& function, const std::string& message);
void log_info(const std::string& file, uint32_t line, const std::string& function, const std::string& message);
void log_debug(const std::string& file, uint32_t line, const std::string& function, const std::string& message);
void log_verbose(const std::string& file, uint32_t line, const std::string& function, const std::string& message);

// clang-format on

}  // namespace SimpleDBus

#define LOG_FATAL(message) SimpleDBus::log_fatal(__FILE__, __LINE__, __PRETTY_FUNCTION__, message)
#define LOG_ERROR(message) SimpleDBus::log_error(__FILE__, __LINE__, __PRETTY_FUNCTION__, message)
#define LOG_WARN(message) SimpleDBus::log_warn(__FILE__, __LINE__, __PRETTY_FUNCTION__, message)
#define LOG_INFO(message) SimpleDBus::log_info(__FILE__, __LINE__, __PRETTY_FUNCTION__, message)
#define LOG_DEBUG(message) SimpleDBus::log_debug(__FILE__, __LINE__, __PRETTY_FUNCTION__, message)
#define LOG_VERBOSE(message) SimpleDBus::log_verbose(__FILE__, __LINE__, __PRETTY_FUNCTION__, message)
