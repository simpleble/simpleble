#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logs an error of SimpleKtBLE through SimpleBLE's logger, as the JNI layer does.
 *
 * SimpleCBLE can configure the logger but has no call to log a message.
 *
 * @param[in] function NUL-terminated name of the callback that failed.
 * @param[in] message NUL-terminated message.
 */
void simplektble_log_error(const char* function, const char* message);

#ifdef __cplusplus
}
#endif
