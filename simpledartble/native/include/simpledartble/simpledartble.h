#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <simplecble/types.h>

#ifdef _WIN32
#define SIMPLEDARTBLE_EXPORT __declspec(dllexport)
#else
#define SIMPLEDARTBLE_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sets the function that posts a message to a Dart port, NativeApi.postCObject.
 */
SIMPLEDARTBLE_EXPORT void simpledartble_initialize(void* post_cobject);

/**
 * @brief Sends every event to the given port from now on.
 *
 * Questions the previous owner didn't answer get their fallback answer.
 *
 * @return An identifier of the claim for simpledartble_release(), or zero if the isolate that claimed the events
 *     before is still running, which keeps them.
 */
SIMPLEDARTBLE_EXPORT int64_t simpledartble_claim(int64_t port);

/**
 * @brief Ends a claim, unless a later one replaced it: no more events are posted, and questions get their fallback.
 *
 * Takes the identifier simpledartble_claim() returned as a pointer, the signature of a NativeFinalizer, which runs
 * it when the isolate group shuts down. The Dart VM must not be posted to after that.
 */
SIMPLEDARTBLE_EXPORT void simpledartble_release(void* claim);

/**
 * @brief Creates a token to register a callback with, unique in this process.
 *
 * Pass the token as the userdata of a callback setter, together with the simpledartble_on_* function of the
 * callback's signature. Each callback posts a message to the claiming port:
 *
 * - [token] for a callback without data.
 * - [token, peripheral] with the address of an owned peripheral handle, released here if no isolate receives it.
 *   Integers are only ever peripheral handles.
 * - [token, bytes] with a copy of the data, as a Uint8List.
 * - [token, text] with a copy of the string.
 * - [token, question, argument] for a callback that waits for an answer, with argument null or a string. Answer
 *   with simpledartble_answer_*.
 *
 * The logging callback posts [token, [level, module, file, line, function, message]] for the token given to
 * simpledartble_log_to().
 */
SIMPLEDARTBLE_EXPORT int64_t simpledartble_token(void);

/** @brief Posts [token]. Takes adapter, peripheral and local characteristic callbacks. */
SIMPLEDARTBLE_EXPORT void simpledartble_on_signal(void* handle, void* userdata);

/** @brief Posts [token, peripheral] for a scan callback, which passes an owned peripheral handle. */
SIMPLEDARTBLE_EXPORT void simpledartble_on_peripheral(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral,
                                                      void* userdata);

/** @brief Posts [token, bytes] for a notification or an indication. */
SIMPLEDARTBLE_EXPORT void simpledartble_on_value(simpleble_peripheral_t peripheral, simpleble_uuid_t service,
                                                 simpleble_uuid_t characteristic, const uint8_t* data,
                                                 size_t data_length, void* userdata);

/** @brief Posts [token, bytes] for a write to a local characteristic. */
SIMPLEDARTBLE_EXPORT void simpledartble_on_bytes(void* handle, const uint8_t* data, size_t data_length,
                                                 void* userdata);

/** @brief Posts [token, text]. Takes client connection and passkey display callbacks. */
SIMPLEDARTBLE_EXPORT void simpledartble_on_text(void* handle, const char* text, void* userdata);

/**
 * @brief Asks for a local characteristic's value with [token, question, null] and waits for the answer.
 *
 * Without an answer, the read gets an empty value.
 */
SIMPLEDARTBLE_EXPORT const uint8_t* simpledartble_on_read(simpleble_local_characteristic_t handle,
                                                          size_t* data_length, void* userdata);

/**
 * @brief Asks for a Dongl pairing passkey with [token, question, null] and waits for the answer.
 *
 * Without an answer, pairing is rejected.
 */
SIMPLEDARTBLE_EXPORT bool simpledartble_on_passkey_request(simpleble_peripheral_t peripheral, char passkey[7],
                                                           void* userdata);

/**
 * @brief Asks whether a Dongl pairing passkey matches with [token, question, passkey] and waits for the answer.
 *
 * Without an answer, pairing is rejected.
 */
SIMPLEDARTBLE_EXPORT bool simpledartble_on_numeric_comparison(simpleble_peripheral_t peripheral, const char* passkey,
                                                              void* userdata);

/** @brief Answers a question with bytes. */
SIMPLEDARTBLE_EXPORT void simpledartble_answer_bytes(int64_t question, const uint8_t* data, size_t data_length);

/** @brief Answers a question with a NUL-terminated string. */
SIMPLEDARTBLE_EXPORT void simpledartble_answer_text(int64_t question, const char* text);

/** @brief Answers a question with true or false. */
SIMPLEDARTBLE_EXPORT void simpledartble_answer_flag(int64_t question, bool flag);

/** @brief Gives a question its fallback answer. */
SIMPLEDARTBLE_EXPORT void simpledartble_answer_none(int64_t question);

/**
 * @brief Sets the token simpledartble_on_log() posts entries with.
 *
 * Register simpledartble_on_log with simpleble_logging_set_callback() after setting it.
 */
SIMPLEDARTBLE_EXPORT void simpledartble_log_to(int64_t token);

/**
 * @brief Posts a log entry for the token of simpledartble_log_to().
 *
 * Takes the level as an int, the size of simpleble_log_level_t, so Dart can pass its address.
 */
SIMPLEDARTBLE_EXPORT void simpledartble_on_log(int level, const char* module, const char* file, uint32_t line,
                                               const char* function, const char* message);

/**
 * @brief Whether SimpleBLE can reach the platform.
 *
 * False only on Android until SimpleFlutterBLE's initializer loads the library, which gives SimpleBLE the JVM.
 */
SIMPLEDARTBLE_EXPORT bool simpledartble_platform_ready(void);

#ifdef __cplusplus
}
#endif
