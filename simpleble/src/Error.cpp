#include <simpleble/Error.h>

#include <utility>

using namespace SimpleBLE;

Error::Error(ErrorCode code, std::unique_ptr<std::exception> exception) : code(code), exception(std::move(exception)) {}
