#include <simpleble/Error.h>

#include <stdexcept>
#include <utility>

using SimpleBLE::ErrorCode;

simpleble_error::simpleble_error(ErrorCode code, std::unique_ptr<std::exception> exception)
    : code(code), exception(std::move(exception)) {}

simpleble_error::simpleble_error(ErrorCode code, const char* message)
    : simpleble_error(code, std::make_unique<std::runtime_error>(message)) {}
