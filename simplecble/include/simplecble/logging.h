#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <simplecble/export.h>

typedef enum {
    SIMPLEBLE_LOG_LEVEL_NONE = 0,
    SIMPLEBLE_LOG_LEVEL_FATAL,
    SIMPLEBLE_LOG_LEVEL_ERROR,
    SIMPLEBLE_LOG_LEVEL_WARN,
    SIMPLEBLE_LOG_LEVEL_INFO,
    SIMPLEBLE_LOG_LEVEL_DEBUG,
    SIMPLEBLE_LOG_LEVEL_VERBOSE
} simpleble_log_level_t;

// clang-format off
typedef void (*simpleble_log_callback_t)(
    simpleble_log_level_t level,
    const char* module,
    const char* file,
    uint32_t line,
    const char* function,
    const char* message
);
// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sets the logging verbosity level.
 *
 * @param[in] level Logging level, from SIMPLEBLE_LOG_LEVEL_NONE through SIMPLEBLE_LOG_LEVEL_VERBOSE.
 */
SIMPLECBLE_EXPORT void simpleble_logging_set_level(simpleble_log_level_t level);

/**
 * @brief Retrieves the logging verbosity level.
 *
 * @return The current logging level.
 */
SIMPLECBLE_EXPORT simpleble_log_level_t simpleble_logging_get_level(void);

/**
 * @brief Sets or clears the logging callback.
 *
 * @param[in] callback Callback to install, or NULL to clear it.
 * @note Callback strings are borrowed and valid only during the callback.
 */
SIMPLECBLE_EXPORT void simpleble_logging_set_callback(simpleble_log_callback_t callback);

/**
 * @brief Checks whether a logging callback is installed.
 *
 * @return true if a logging callback is installed, otherwise false.
 */
SIMPLECBLE_EXPORT bool simpleble_logging_has_callback(void);

/**
 * @brief Installs the default logging callback that writes to standard output.
 *
 */
SIMPLECBLE_EXPORT void simpleble_logging_log_default_stdout(void);

/**
 * @brief Installs the default logging callback that appends to a timestamp-named file.
 *
 */
SIMPLECBLE_EXPORT void simpleble_logging_log_default_file(void);

/**
 * @brief Installs the default logging callback that appends to the specified file.
 *
 * @param[in] path NUL-terminated file path, or NULL to use a timestamp-named file.
 */
SIMPLECBLE_EXPORT void simpleble_logging_log_default_file_path(const char* path);

#ifdef __cplusplus
}
#endif
