#pragma once

#include <simplecble/export.h>
#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Retrieves the code identifying the stored exception or binding error.
 * @param[in] error Valid, non-NULL error to inspect.
 * @return The error code.
 */
SIMPLECBLE_EXPORT simpleble_err_t simpleble_error_code(const simpleble_error_t* error);

/**
 * @brief Retrieves the error message.
 * @param[in] error Valid, non-NULL error to inspect.
 * @return Borrowed, NUL-terminated text.
 * @note The text remains valid until the error is released, either explicitly
 *     or by the next call using the same error variable.
 */
SIMPLECBLE_EXPORT const char* simpleble_error_message(const simpleble_error_t* error);

/**
 * @brief Releases an error and its diagnostic data, then sets the handle to NULL.
 * @param[in,out] error Required, non-NULL pointer to NULL or an owned error.
 * @note Use this function instead of simpleble_free().
 */
SIMPLECBLE_EXPORT void simpleble_error_release(simpleble_error_t** error);

#ifdef __cplusplus
}
#endif
